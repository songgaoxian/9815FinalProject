/*
implement historical data servic for bond position
author: Gaoxian Song
*/
#ifndef HistoricalDataInquiry_HPP
#define HistoricalDataInquiry_HPP

#include "historicaldataservice.hpp"
#include <map>

//connector for historical data service for bond position
//paired with persist key
class BondIqHistoricalConnector: public Connector<pair<string,Inquiry<Bond> > >
{
public:
  // Publish data to the Connector
  virtual void Publish(pair<string, Inquiry<Bond> > &data);
};
//historical dataservice for bond position
//keyed on record, not product
class BondIqHistoricalData: public HistoricalDataService<Inquiry<Bond> >
{
private:
  int counter;//count record to determine the key
  map<string, Inquiry<Bond> > bondHistoricalCache;
  vector<ServiceListener<Inquiry<Bond> >* > bondListeners;//listeners
  BondIqHistoricalConnector& b_historical;//connector to output file
public:
  BondIqHistoricalData(BondIqHistoricalConnector& src):b_historical(src){counter=1;}//constructor
  // Get data on our service given a key
  virtual Inquiry<Bond>& GetData(string key){return bondHistoricalCache.find(key)->second;}

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Inquiry<Bond> &data){}//do nothing

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Inquiry<Bond> > *listener){bondListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<Inquiry<Bond> >* >& GetListeners() const{return bondListeners;}
  //persist data to a store
  virtual void PersistData(string persistKey, const Inquiry<Bond>& data){
  	pair<string, Inquiry<Bond> > dataPair=make_pair(persistKey,data);//construct data pair
  	b_historical.Publish(dataPair);//publish data to file
  }
  //set key for persist data
  void SetPersistKey(Inquiry<Bond>& data){
  	//get key from counter and increment counter
  	string k=to_string(counter); ++counter;
  	Inquiry<Bond> data_copy=data; PersistData(k,data_copy);
  }
};

class BondIqHistoricalListener: public ServiceListener<Inquiry<Bond> >
{
private:
	BondIqHistoricalData& b_historical_data;//to flow into
public:
	BondIqHistoricalListener(BondIqHistoricalData& src): b_historical_data(src){}
	// Listener callback to process an update event to the Service
  virtual void ProcessUpdate(Inquiry<Bond> &data){
    b_historical_data.SetPersistKey(data);
    InquiryState s=data.GetState();//get state
    if(s==QUOTED){
      data.SetState(DONE);//update to done 
      b_historical_data.SetPersistKey(data);//flow updated data
    }
  }

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Inquiry<Bond> &data){}

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(Inquiry<Bond> &data){b_historical_data.SetPersistKey(data);}
};

//implement publish
void BondIqHistoricalConnector::Publish(pair<string, Inquiry<Bond> > &data1){
	ofstream file;
  file.open("./Output/Historical/allinquiries.txt",ios_base::app);//open the file to append
   //get key
	string k=data1.first;
	file<<k<<",";
  Inquiry<Bond> data=data1.second;//get price stream
  string iqid=data.GetInquiryId();//get inquiry id
  file<<iqid<<",";
   Bond bnd=data.GetProduct();//get prodcut of data
   string bondid=bnd.GetProductId();//get bond cusip
   file<<bondid<<",";//write cusip to file
   Side theside=data.GetSide();//get side
   if(theside==BUY){
    file<<"BUY,";
   }
   else{
    file<<"SELL,";
   }
   long q=data.GetQuantity();//get quantity
   file<<to_string(q)<<",";
   double price=data.GetPrice();//get bid price
   string p_str=PriceProcess(price);//convert to string
   file<<p_str<<",";//write to file
   InquiryState s=data.GetState();//get inquiry state
   switch(s){
    case RECEIVED:
      file<<"RECEIVED\n"; break;
    case QUOTED:
      file<<"QUOTED\n";break;
    case REJECTED:
      file<<"REJECTED\n";break;
    case CUSTOMER_REJECTED:
      file<<"CUSTOMER_REJECTED\n";break;
    case DONE:
      file<<"DONE\n";break;
   }
}

#endif