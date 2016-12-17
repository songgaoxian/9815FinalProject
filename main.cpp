#include "tradebookingservice.hpp"
#include "positionservice.hpp"
#include "pricingservice.hpp"
#include "riskservice.hpp"
#include "marketdataservice.hpp"
#include "executionservice.hpp"
#include "streamingservice.hpp"
#include "inquiryservice.hpp"
#include "historicaldataposition.hpp"
#include "historicaldatarisk.hpp"
#include "historicalexecutionservice.hpp"
#include "historicalstreamingservice.hpp"
#include "historicalinquiryservice.hpp"

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
    //numoftrades is number of trades to flow into tradebook service
    //numofprice is number of prices to flow into bondpriceservice
    //numofmarket is number of marketdata to flow into market data service
    //numofiq is number of inquiries to flow into inquiry service
    int numOftrades=18, numofprice=36, numofmarket=36, numofiq=36;

	map<string, Bond> m_bond=GetBonds();//get a map of bonds
	vector<string> bids; //store bond ids
    map<string, double> m_bond_pv01;
	for(map<string, Bond>::iterator it=m_bond.begin(); it!=m_bond.end();++it){
		bids.push_back(it->first);//push bond ids to bids
	}
    //assign reasonable values as pv01
    m_bond_pv01[bids[0]]=0.295;
    m_bond_pv01[bids[1]]=0.102;
    m_bond_pv01[bids[2]]=0.031;
    m_bond_pv01[bids[3]]=0.017;
    m_bond_pv01[bids[4]]=0.0707;
    m_bond_pv01[bids[5]]=0.0455;
    PV01<Bond> temp(m_bond[bids[0]],0,0);
    //configure services, listeners, etc and link them together
    BondTradeBookService bt_service;//construct trade book service
    BondTradeBookingConnector bt_connector; //construct trade book connector
    BondPositionService bposition; //construct bond position service
    BondRiskService bndrisk(m_bond_pv01, m_bond); //construct bond risk service
    BondRiskHistoricalConnector b_risk_connector; //construct bond risk historical data connector
    BondRiskHistoricalData b_risk_data(b_risk_connector); //construct bond risk historical data service
                                                          //and link with corresponding connector
    BondRiskRecordListener b_risk_record_listen(b_risk_data);//construct risk record listener
    BondPV01HistoricalListener* b_pv01_listen=new BondPV01HistoricalListener(temp);//construct pv01 listener for historical data
                                                                            //the temp will be modifed later
    bndrisk.AddListener(b_pv01_listen);//add pv01 listener to bond risk service
    //construct bond sectors risk listener and link with pv01 lister and risk record listener for historical data service
    BondSectorsRiskListener* b_sector_listen=new BondSectorsRiskListener(*b_pv01_listen,b_risk_record_listen);
    bndrisk.AddListener(b_sector_listen);//add bond sectors listener to risk service
    //construct bond position connector for historical data
    BondPositionHistoricalConnector bp_his_connector;
    //construct bond position historical data service and link with connector
    BondPositionHistoricalData bp_his_data(bp_his_connector);
    //construct bond position listener for historical data service and linke with bond historical position servce
    BondPositionHistoricalListener* bp_his_listener=new BondPositionHistoricalListener(bp_his_data);
    //construct bond position listener and link with risk service
    BondPositionServiceListener* bnd_pos_listener=new BondPositionServiceListener(bndrisk);
    //add positionlisteners to bond position service
    bposition.AddListener(bnd_pos_listener);
    bposition.AddListener(bp_his_listener);
    //construct trade listener and link with bond position service
    BondTradeListener* ptr_bt_listen=new BondTradeListener(bposition);
    //add trade listener to tradebooking service
    bt_service.AddListener(ptr_bt_listen);
    //flow trade data to trade book connector, no more than 60
    for(int i=0;i<numOftrades;++i){
      bt_connector.Subscribe(bt_service, m_bond);
    }
    //construct bond price service
    BondPriceService bp_service;
    //construct price connector
    BondPriceConnector bp_connector;
    //construct bond algo stream service
    BondAlgoStreamingService b_algo_stream;
    //construct bond price listener and link with algo stream service
    BondPriceListener* b_price_listener=new BondPriceListener(b_algo_stream);
    //add bond price listener to bond price serivce
    bp_service.AddListener(b_price_listener);
    //construct bond stream service
    BondStreamingService b_stream_service;
    //construct bond stream connector for historical data
    BondStreamHistoricalConnector b_stream_connect;
    //construct bond stream historical service and link with connector
    BondStreamHistoricalData b_stream_data(b_stream_connect);
    //construct bond stream listener for historical data service and link with bond stream historical service
    BondStreamHistoricalListener* b_stream_listen=new BondStreamHistoricalListener(b_stream_data);
    //add listener to bond stream service
    b_stream_service.AddListener(b_stream_listen);
    //construct bond algo stream listener and link with bond stream service
    BondAlgoStreamListener* b_algo_stream_listener=new BondAlgoStreamListener(b_stream_service);
    //add bond algo stream listener to bond algo stream service
    b_algo_stream.AddListener(b_algo_stream_listener);
    //flow price data to bond price connector
    for(int i=1;i<=numofprice;++i){
      bp_connector.Subscribe(bp_service,m_bond);
    }
    //test the update pv01 function
    bndrisk.UpdateBondPV01(bids[2],0.03);
    //construct bond execution service
    BondExecutionService b_exe_service;
    //construct bond execution connector for historical data
    BondExecutionHistoricalConnector b_exe_connect;
    //construct bond execution historical data service and link with connector
    BondExecutionHistoricalData b_exe_data(b_exe_connect);
    //construct bond executionorder listener and link with bond execution historical data service
    BondExecutionHistoricalListener* b_exe_listen=new BondExecutionHistoricalListener(b_exe_data);
    //construct bond algoexecution listener and link with bond execution service
    BondAlgoExecutionListener* b_algo_listener=new BondAlgoExecutionListener(b_exe_service);
    //add bond execution listener to bond execution service
    b_exe_service.AddListener(b_exe_listen);
    //construct market data service
    BondMarketDataService bm_ds;
    //construct bond algo execution service
    BondAlgoExecutionService b_algo_exe;
    //add algo listener to bond algo execution service
    b_algo_exe.AddListener(b_algo_listener);
    //construct bond market data listener and link with bond algo execution service
    BondMarketDataListeners* b_mkt_listener=new BondMarketDataListeners(b_algo_exe);
    //add bond market data listener to market data service
    bm_ds.AddListener(b_mkt_listener);

    //construct bond market data connector
    BondMarketDataConnector bm_connect;
    //flow market data to bond market data service
    for(int i=0;i<numofmarket;++i){
      bm_connect.Subscribe(bm_ds,m_bond);
    }
    //construct inquiry connector for publish
    BondPublishIqConnector b_publish;
    //construct inquiry connector for historical data
    BondIqHistoricalConnector b_iq_hist_connect;
    //construct bond inquiry historical data service and link with connector
    BondIqHistoricalData b_iq_data(b_iq_hist_connect);
    //construct bond inquiry historical listener and link with bond inquiry historical data service
    BondIqHistoricalListener* b_iq_hist_listen=new BondIqHistoricalListener(b_iq_data);
    //construct bond inquiry service and link with connector
    BondInquiryService b_inquire(b_publish);
    //construct bond inquiry service listener and link with bond inquiry service
    BondInquiryListener* b_iq_listen=new BondInquiryListener(b_inquire);
    //add listeners to bond inquiry service
    b_inquire.AddListener(b_iq_hist_listen);
    b_inquire.AddListener(b_iq_listen);
    //construct bond inquiry connector
    BondInquiryConnector b_iq_connect;
    //flow data into bond inquiry service, no more than 60
    for(int i=0;i<numofiq;++i){
      b_iq_connect.Subscribe(b_inquire,m_bond);
    }
    return 0;
}