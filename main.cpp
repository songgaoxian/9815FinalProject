#include "tradebookingservice.hpp"
#include "positionservice.hpp"
#include "pricingservice.hpp"
#include "riskservice.hpp"
#include "marketdataservice.hpp"

map<string, Bond> GetBonds(){
	map<string, Bond> m_bond;
	ifstream file("./Input/bonds.txt");//construct input file
    string line;//store each read line
    getline(file,line);//read header line
    while(getline(file,line)){
    	vector<string> bondlines;//to store bond info
    	boost::split(bondlines,line,boost::is_any_of(","));//split line
    	string cusip=bondlines[0];//get cusip
    	float coupon=stof(bondlines[1],nullptr);//get coupon
    	string ticker=bondlines[2];//get ticker
    	date maturity(from_simple_string(bondlines[3]));//get maturity
    	Bond thebond(cusip,CUSIP,ticker,coupon,maturity);//construct bond
    	m_bond.insert(std::make_pair(cusip,thebond));//add this entry to m_bond
    }
    return m_bond;
}

int main(){
	map<string, Bond> m_bond=GetBonds();//get a map of bonds
	vector<string> bids; //store bond ids
    map<string, double> m_bond_pv01;
	for(map<string, Bond>::iterator it=m_bond.begin(); it!=m_bond.end();++it){
		bids.push_back(it->first);//push bond ids to bids
	}
    m_bond_pv01[bids[0]]=0.295;
    m_bond_pv01[bids[1]]=0.102;
    m_bond_pv01[bids[2]]=0.031;
    m_bond_pv01[bids[3]]=0.017;
    m_bond_pv01[bids[4]]=0.0707;
    m_bond_pv01[bids[5]]=0.0455;
    //configure tradebook, position and risk
    BondTradeBookService bt_service;
    BondTradeBookingConnector bt_connector;
    BondPositionService bposition;
    BondRiskService bndrisk(m_bond_pv01);
    BondPositionServiceListener* bnd_pos_listener=new BondPositionServiceListener(bndrisk);
    bposition.AddListener(bnd_pos_listener);
    BondTradeListener* ptr_bt_listen=new BondTradeListener(bposition);
    bt_service.AddListener(ptr_bt_listen);
    

    bt_connector.Subscribe(bt_service, m_bond);
    bt_connector.Subscribe(bt_service,m_bond);
    bt_service.GetData("0");
    Trade<Bond> bont1=bt_connector.Subscribe(bt_service,m_bond);
    bt_service.BookTrade(bont1);
    
    BondPriceService bp_service;
    BondPriceConnector bp_connector;
    Price<Bond> pbnd=bp_connector.Subscribe(bp_service,m_bond);
    cout<<pbnd.GetMid()<<"\n";

    bndrisk.GetData(bids[2]);
    vector<Bond> v_bond{m_bond[bids[2]],m_bond[bids[3]]};
    BucketedSector<Bond> buck_bnd(v_bond,"hello");
    PV01<BucketedSector<Bond> > sector_bnd=bndrisk.GetBucketedRisk(buck_bnd);
    cout<<sector_bnd.GetPV01()<<"\n";

    //marketdataservice has debug-purpose cout
    BondMarketDataService bm_ds;
    BondMarketDataConnector bm_connect;
    for(int i=0;i<12;++i){
      bm_connect.Subscribe(bm_ds,m_bond);
    }
    BidOffer bid_offer=bm_ds.GetBestBidOffer(bids[0]);
    Order od=bid_offer.GetBidOrder();
    cout<<"bondid="<<bids[0]<<" has best bid="<<od.GetPrice()<<"\n";

    return 0;
}