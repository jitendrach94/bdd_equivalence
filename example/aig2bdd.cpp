#include <SimpleBddMan.hpp>
#include <CuddMan.hpp>
#include <BuddyMan.hpp>
#include <CacBddMan.hpp>
#include <AtBddMan.hpp>

#include <NtkBdd.hpp>
#include <BddGraph.hpp>
#include <mockturtle/mockturtle.hpp>
#include <lorina/lorina.hpp>

#include <string>
#include <chrono>



template <typename node>
auto run( Bdd::BddMan<node> & bdd, mockturtle::aig_network & aig, mockturtle::klut_network * klut, bool dvr, std::vector<std::string> & pi_names, std::vector<std::string> & po_names, std::string dotname, bool cedge, int verbose ) {
  if(dvr) {
    bdd.Dvr();
  }
  auto start = std::chrono::system_clock::now();
  auto vNodes = Bdd::Aig2Bdd( aig, bdd, verbose > 1 );
  auto end = std::chrono::system_clock::now();
  if(verbose) {
    std::cout << "time : " << std::chrono::duration_cast<std::chrono::milliseconds>( end - start ).count() << " ms" << std::endl;
    bdd.PrintStats( vNodes );
    std::cout<<"===================================================="<<std::endl;
    if(dvr) {
      std::cout << "Ordering :" << std::endl;
      std::vector<int> v( aig.num_pis() );
      for ( int i = 0; i < aig.num_pis(); i++ )
	{
	  v[bdd.Level( i )] = i;
	}
      for ( int i : v )
	{
	  if ( pi_names.empty() || pi_names[i].empty() ) {
	    std::cout << "pi" << i + 2 << " "; // + 2 matches mocuturtle write_blif
	  }
	  else {
	    std::cout << pi_names[i] << " ";
	  }
	}
      std::cout << std::endl;
    }
  }
  if(klut) {
    Bdd::Bdd2Ntk( *klut, bdd, vNodes, cedge );
  }
  if(!dotname.empty()) {
    Bdd::Bdd2Dot( dotname, bdd, vNodes, pi_names, po_names, cedge );
  }
  return vNodes;
}


template <typename node>
bool CheckEQ(Bdd::BddMan<node> & bdd, std::vector<node> const & vNodes, int const nPI, std::vector<std::string> const & pi_names)
{
	std::map<std::string, std::string> temp_val;
	int visited = 0;
	int path = 0;
	for(auto in : pi_names)
	{
		std::string in1;
		std::string in2;
		bdd.splitIn(in1, in2, in);
		temp_val.insert({in1, "U"});
	}
	auto start1 = std::chrono::system_clock::now();
	for(auto i = 0; i < vNodes.size(); i++)
	{
	  	std::map<std::string, std::string> temp = temp_val;
	  	std::vector<int> cex;
	  	for(auto i = 0; i < nPI; i++)
	  	{
	  		cex.push_back(1);
	  	}
	  	if(bdd.GetCEX(vNodes.at(i), cex, pi_names, temp, visited, path))
	  	{
	  		return false;
	  	}
	}
	std::cout<<"Visited nodes: "<<visited<<std::endl;
	std::cout<<"Visited paths: "<<path<<std::endl;
	auto end1 = std::chrono::system_clock::now();
	std::cout << "time : " << std::chrono::duration_cast<std::chrono::milliseconds>( end1 - start1 ).count() << " ms" << std::endl;
	return true;
}


int main( int argc, char ** argv ) {
  std::string pFile;
  std::string blifname;
  std::string dotname;
  int package = 3;
  bool supportname = 1;
  bool dvr = 0;
  int verbose = 1;
  int pverbose = 0;
  bool cedge = 0;
  
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] != '-') {
      if(pFile.empty()) {
	pFile = argv[i];
	continue;
      }
      else {
	std::cerr << "invalid option " << argv[i] << std::endl;
	return 1;
      }
    }
    else if(argv[i][1] == '\0') {
      std::cerr << "invalid option " << argv[i] << std::endl;
      return 1;
    }
    int i_ = i;
    for(int j = 1; argv[i_][j] != '\0'; j++) {
      if(i != i_) {
	std::cerr << "invalid option " << argv[i_] << std::endl;
	return 1;
      }
      switch(argv[i_][j]) {
      case 'c':
	cedge ^= 1;
	break;
      case 'd':
	if(i+1 >= argc) {
	  std::cerr << "-d must be followed by file name" << std::endl;
	  return 1;
	}
	dotname = argv[++i];
	break;
      case 'o':
	if(i+1 >= argc) {
	  std::cerr << "-o must be followed by file name" << std::endl;
	  return 1;
	}
	blifname = argv[++i];
	break;
      case 'p':
	try {
	  package = std::stoi(argv[++i]);
	}
	catch(...) {
	  std::cerr << "-n must be followed by integer" << std::endl;
	  return 1;
	}
	break;
      case 's':
	supportname ^= 1;
	break;
      case 'r':
	dvr ^= 1;
	break;
      case 'v':
	try {
	  verbose = std::stoi(argv[++i]);
	}
	catch(...) {
	  std::cerr << "-v must be followed by integer" << std::endl;
	  return 1;
	}
	break;
      case 'V':
	try {
	  pverbose = std::stoi(argv[++i]);
	}
	catch(...) {
	  std::cerr << "-V must be followed by integer" << std::endl;
	  return 1;
	}
	break;
      case 'h':
	std::cout << "usage : aig2bdd <options> circuit_miter.aig" << std::endl;
	std::cout << "\t	: this method is to check the equivalence between two circuits" << std::endl;
	std::cout << "\t-h       : show this usage" << std::endl;
	std::cout << "\t-p <int> : package [default = " << package << "]" << std::endl;
	std::cout << "\t           \t0 : cudd" << std::endl;
	std::cout << "\t           \t1 : buddy" << std::endl;
	std::cout << "\t           \t2 : cacbdd" << std::endl;
	std::cout << "\t           \t3 : simplebdd" << std::endl;
	std::cout << "\t           \t4 : custombdd" << std::endl;
	std::cout << "\t-V <int> : toggle verbose information inside BDD package [default = " << pverbose << "]" << std::endl;
	return 0;
      default:
	std::cerr << "invalid option " << argv[i] << std::endl;
	return 1;
      }
    }
  }
  if(pFile.empty()) {
    std::cerr << "specify aigname" << std::endl;
    return 1;
  }
  auto total_s = std::chrono::system_clock::now();
  mockturtle::aig_network aig;
  mockturtle::NameMap<mockturtle::aig_network> namemap;
  std::vector<std::string> pi_names;
  std::vector<std::string> po_names;
  //if(supportname) {
    lorina::read_aiger(pFile, mockturtle::aiger_reader(aig, &namemap));
    aig.foreach_pi([&](auto pi) {
		     auto namevec = namemap[aig.make_signal(pi)];
		     if(namevec.empty()) {
		       pi_names.push_back("");
		       return;
		     }
		     pi_names.push_back(namevec[0]);
		   });
    aig.foreach_po([&](auto po) {
		     auto namevec = namemap[po];
		     if(namevec.empty()) {
		       po_names.push_back("");
		       return;
		     }
		     po_names.push_back(namevec[namevec.size() - 1]);
		   });
  /*}
  else {
    lorina::read_aiger(pFile1, mockturtle::aiger_reader(aig1));
  }
  for(auto item : pi_names)
  {
  	std::cout<<item<<"\t";
  }
  std::cout<<std::endl;*/
  
  
  mockturtle::klut_network * klut = NULL;
  if(!blifname.empty()) {
    klut = new mockturtle::klut_network;
  }
  try {
  switch(package) {
  case 0:
    {
      Bdd::CuddMan bdd( aig.num_pis(), pverbose );
      auto vNodes = run( bdd, aig, klut, dvr, pi_names, po_names, dotname, cedge, verbose );
	  if(CheckEQ(bdd, vNodes, aig.num_pis(), pi_names))
	  {
	  	std::cout<<"EQUIVALENT"<<std::endl;
	  }else{
	  	std::cout<<"Non-EQUIVALENT"<<std::endl;
	  }
    }
    break;
  case 1:
    {
      if(cedge) {
	std::cerr << "the package doesn't use complemented edges" << std::endl;
      }
      Bdd::BuddyMan bdd( aig.num_pis(), pverbose );
      auto vNodes = run( bdd, aig, klut, dvr, pi_names, po_names, dotname, 0, verbose );
      std::cout<<"Equivalence checking not implemented"<<std::endl;
    }
    break;
  case 2:
    {
      if(pverbose) {
	std::cerr << "the package doesn't have verbose system" << std::endl;
      }
      Bdd::CacBddMan bdd( aig.num_pis() );
      auto vNodes = run( bdd, aig, klut, dvr, pi_names, po_names, dotname, cedge, verbose );
      std::cout<<"Equivalence checking not implemented"<<std::endl;
    }
    break;
  case 3:
    {
      Bdd::SimpleBddMan bdd( aig.num_pis(), pverbose );
      auto vNodes = run( bdd, aig, klut, dvr, pi_names, po_names, dotname, cedge, verbose );
	  if(CheckEQ(bdd, vNodes, aig.num_pis(), pi_names))
	  {
	  	std::cout<<"EQUIVALENT"<<std::endl;
	  }else{
	  	std::cout<<"Non-EQUIVALENT"<<std::endl;
	  }
	 }
    break;
  case 4:
    {
      Bdd::AtBddMan bdd( aig.num_pis(), pverbose );
      auto vNodes = run( bdd, aig, klut, dvr, pi_names, po_names, dotname, cedge, verbose );
	  if(CheckEQ(bdd, vNodes, aig.num_pis(), pi_names))
	  {
	  	std::cout<<"EQUIVALENT"<<std::endl;
	  }else{
	  	std::cout<<"Non-EQUIVALENT"<<std::endl;
	  }
	  
    }
    break;
  default:
    std::cerr << "unknown package number " << package << std::endl;
    return 1;
  }
  }  
  catch ( char const * e ) {
    std::cerr << e << std::endl;
    return 1;
  }
  catch ( ... ) {
    std::cerr << "error" << std::endl;
    return 1;
  }
  auto total_e = std::chrono::system_clock::now();
  std::cout << "Total time : " << std::chrono::duration_cast<std::chrono::milliseconds>( total_e - total_s ).count() << " ms" << std::endl;
  if(klut) {
    if(supportname) {
      mockturtle::names_view klut_{*klut};
      klut_.foreach_pi([&](auto pi, int i) {
			 if(!pi_names[i].empty()) {
			   klut_.set_name(klut_.make_signal(pi), pi_names[i]);
			 }
		       });
      for(int i = 0; i < klut_.num_pos(); i++) {
	if(!po_names[i].empty()) {
	  klut_.set_output_name( i, po_names[i] );
	}
      }
      mockturtle::write_blif( klut_, blifname );
    }
    else {
      mockturtle::write_blif( *klut, blifname );
    }
    delete klut;
  }

  return 0;
}
