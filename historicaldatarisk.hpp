/*
implement historical data servic for bond risk
author: Gaoxian Song
*/
#ifndef HistoricalDataRisk_HPP
#define HistoricalDataRisk_HPP

#include "historicaldataservice.hpp"
#include <map>

//risk record containing pv01 for all securities and three sectors
class BondRiskRecord
{
public:
  string persistKey;//the persist key
  PV01<Bond> b_pv01;//the bond's pv01 to be updated or added
  PV01<BucketedSector<Bond> > front_end, belly, long_end;//three sectors to record
  BondRiskRecord(PV01<Bond>& src1, PV01<BucketedSector<Bond> >& f1, PV01<BucketedSector<Bond> >& b1, PV01<BucketedSector<Bond> >& l1):b_pv01(src1),front_end(f1),belly(b1),long_end(l1), persistKey("123"){}//ctor, avoid null string
};

//connector for historical data service for bond position
//paired with persist key
class BondRiskHistoricalConnector: public Connector<BondRiskRecord>
{
public:
  // Publish data to the Connector
  virtual void Publish(BondRiskRecord &data);
};

//historical dataservice for bond risk
//keyed on record, not product
class BondRiskHistoricalData: public HistoricalDataService<BondRiskRecord>
{
private:
  int counter;//count record to determine the key
  map<string, BondRiskRecord> bondRecordRiskCache;
  vector<ServiceListener<BondRiskRecord>* > bondRiskListeners;//listeners
  BondRiskHistoricalConnector& b_risk_historical;//connector to output file
public:
  BondRiskHistoricalData(BondRiskHistoricalConnector& src):b_risk_historical(src){counter=1;}//constructor
  // Get data on our service given a key
  virtual BondRiskRecord& GetData(string key){return bondRecordRiskCache.find(key)->second;}

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(BondRiskRecord &data){}//do nothing

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<BondRiskRecord> *listener){bondRiskListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<BondRiskRecord>* >& GetListeners() const{return bondRiskListeners;}
  //persist data to a store
  virtual void PersistData(string persistKey, const BondRiskRecord& data){
    BondRiskRecord datacopy=data;//construct data pair
  	b_risk_historical.Publish(datacopy);//publish data to file
  }
  //set key for persist data
  void SetPersistKey(BondRiskRecord& data){
  	//get key from counter and increment counter
  	string k=to_string(counter); ++counter;data.persistKey=k;
  	BondRiskRecord data_copy=data; PersistData(k,data_copy);
  }
};

class BondRiskRecordListener: public ServiceListener<BondRiskRecord>
{
private:
	BondRiskHistoricalData& b_historical_data;//to flow into
public:
	BondRiskRecordListener(BondRiskHistoricalData& src): b_historical_data(src){}
	// Listener callback to process an add event to the Service
  virtual void ProcessAdd(BondRiskRecord &data){}

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(BondRiskRecord &data){}

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(BondRiskRecord &data){b_historical_data.SetPersistKey(data);}
  virtual void SetUpdate(PV01<Bond>& data1, SectorsRisk& data2);
};


//listen to pv01 changes
class BondPV01HistoricalListener: public ServiceListener<PV01<Bond> >
{
private:
  PV01<Bond> theData;
  bool needProcessed;
public:
  BondPV01HistoricalListener(PV01<Bond>& data_):theData(data_),needProcessed(false){}
  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(PV01<Bond> &data){
    theData=data;needProcessed=true;
  }

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(PV01<Bond> &data){}

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(PV01<Bond> &data){}
  PV01<Bond> GetData(){return theData;}//get data
  void SetProcessed(bool s){needProcessed=s;}//update the needprocessed status
  bool GetProcessed(){return needProcessed;} //return needprocessed status
};
//listen to sector changes
class BondSectorsRiskListener: public ServiceListener<SectorsRisk>
{
private:
  BondPV01HistoricalListener& b_pv01_listener;
  BondRiskRecordListener& b_risk_record_listener;
public:
  BondSectorsRiskListener(BondPV01HistoricalListener& src1, BondRiskRecordListener& src2): b_pv01_listener(src1),b_risk_record_listener(src2) {}
  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(SectorsRisk &data){}

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(SectorsRisk &data){}

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(SectorsRisk &data);
};

void BondSectorsRiskListener::ProcessUpdate(SectorsRisk &data){
    bool status=b_pv01_listener.GetProcessed();//get status
    if(status){
      b_pv01_listener.SetProcessed(false);//update the status
      PV01<Bond> bnd_pv01=b_pv01_listener.GetData(); //get the data
      b_risk_record_listener.SetUpdate(bnd_pv01,data);
    }
  }

void BondRiskHistoricalConnector::Publish(BondRiskRecord& data){
  ofstream file;
  file.open("./Output/Historical/risk.txt",ios_base::app);//open the file to append
   //get key
  string k=data.persistKey;
  file<<k<<",";
  //get pv01
  PV01<Bond> thepv01=data.b_pv01;
  //get bond
  Bond bnd=thepv01.GetProduct();
  //get bond id
  string bondid=bnd.GetProductId();
  file<<bondid<<",";
  //get pv01
  double pv01_d=thepv01.GetPV01();
  //get quantity
  long qty=thepv01.GetQuantity();
  file<<to_string(qty)<<",";
  //get different bucked sectors
  PV01<BucketedSector<Bond> > sector_pv01=data.front_end;
  //get pv01
  pv01_d=sector_pv01.GetPV01();
  file<<to_string(pv01_d)<<",";
  sector_pv01=data.belly;
  pv01_d=sector_pv01.GetPV01();
  file<<to_string(pv01_d)<<",";
  sector_pv01=data.long_end;
  pv01_d=sector_pv01.GetPV01();
  file<<to_string(pv01_d)<<"\n";
}

void BondRiskRecordListener::SetUpdate(PV01<Bond>& data1, SectorsRisk& data2){
    BondRiskRecord bnd_risk_record(data1,std::get<0>(data2),std::get<1>(data2),std::get<2>(data2));//construct the record
    //invoke processupdate
    ProcessUpdate(bnd_risk_record);
  }

#endif