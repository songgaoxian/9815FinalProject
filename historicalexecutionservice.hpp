/*
implement historical data servic for bond position
author: Gaoxian Song
*/
#ifndef HistoricalDataExecution_HPP
#define HistoricalDataExecution_HPP

#include "historicaldataservice.hpp"
#include <map>

//connector for historical data service for bond position
//paired with persist key
class BondExecutionHistoricalConnector: public Connector<pair<string,ExecutionOrder<Bond> > >
{
public:
  // Publish data to the Connector
  virtual void Publish(pair<string, ExecutionOrder<Bond> > &data);
};
//historical dataservice for bond position
//keyed on record, not product
class BondExecutionHistoricalData: public HistoricalDataService<ExecutionOrder<Bond> >
{
private:
  int counter;//count record to determine the key
  map<string, ExecutionOrder<Bond> > bondHistoricalCache;
  vector<ServiceListener<ExecutionOrder<Bond> >* > bondListeners;//listeners
  BondExecutionHistoricalConnector& b_historical;//connector to output file
public:
  BondExecutionHistoricalData(BondExecutionHistoricalConnector& src):b_historical(src){counter=1;}//constructor
  // Get data on our service given a key
  virtual ExecutionOrder<Bond>& GetData(string key){return bondHistoricalCache.find(key)->second;}

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(ExecutionOrder<Bond> &data){}//do nothing

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<ExecutionOrder<Bond> > *listener){bondListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<ExecutionOrder<Bond> >* >& GetListeners() const{return bondListeners;}
  //persist data to a store
  virtual void PersistData(string persistKey, const ExecutionOrder<Bond>& data){
  	pair<string, ExecutionOrder<Bond> > dataPair=make_pair(persistKey,data);//construct data pair
  	b_historical.Publish(dataPair);//publish data to file
  }
  //set key for persist data
  void SetPersistKey(ExecutionOrder<Bond>& data){
  	//get key from counter and increment counter
  	string k=to_string(counter); ++counter;
  	ExecutionOrder<Bond> data_copy=data; PersistData(k,data_copy);
  }
};

class BondExecutionHistoricalListener: public ServiceListener<ExecutionOrder<Bond> >
{
private:
	BondExecutionHistoricalData& b_historical_data;//to flow into
public:
	BondExecutionHistoricalListener(BondExecutionHistoricalData& src): b_historical_data(src){}
	// Listener callback to process an update event to the Service
  virtual void ProcessUpdate(ExecutionOrder<Bond> &data){}

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(ExecutionOrder<Bond> &data){}

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(ExecutionOrder<Bond> &data){b_historical_data.SetPersistKey(data);}
};

//implement publish
void BondExecutionHistoricalConnector::Publish(pair<string, ExecutionOrder<Bond> > &data){
	ofstream file;
  file.open("./Output/Historical/executions.txt",ios_base::app);//open the file to append
   //get key
	string k=data.first;
	file<<k<<",";
  ExecutionOrder<Bond> exe_order=data.second;//get the execution order
  string orderid=exe_order.GetOrderId(); //get order id
   file<<orderid<<",";//write orderid to file
   Bond bnd=exe_order.GetProduct();//get prodcut of the order
   string bondid=bnd.GetProductId();//get bond cusip
   file<<bondid<<",";//write cusip to file
   PricingSide side=exe_order.GetSide();//Get side
   if(side==BID){
    file<<"BID,";//write side to file
   }
   else{
    file<<"OFFER,";//write side to file
   }
   OrderType o_type=exe_order.GetOrderType();//get order type
   //write corresponding type to file
   switch(o_type){
    case FOK: file<<"FOK,";
              break;
    case IOC: file<<"IOC,";
              break;
    case MARKET: file<<"MARKET,";
                 break;
    case LIMIT: file<<"LIMIT,";
                 break;
    case STOP: file<<"STOP,";
               break;
   }
   long visible=exe_order.GetVisibleQuantity();//get visible qty
   file<<to_string(visible)<<",";//write to file
   long hidden=exe_order.GetHiddenQuantity();//get hidden qty
   file<<to_string(hidden)<<",";//write to file
   double p=exe_order.GetPrice();//get price
   int part1=int(p);//get the integer part of price
   double p2=p-double(part1);//get the decimal part of price
   int part2=int(p2*32);//get part2 of price
   double p3=p2-double(part2)/32;//get last part of price
   int part3=round(p3*256);//get part3 of price
   string p_str;//store the string of price
   if(part2>=10){
     p_str=to_string(part1)+"-"+to_string(part2)+to_string(part3);//construct price string
   }
   else{
    p_str=to_string(part1)+"-"+"0"+to_string(part2)+to_string(part3);
   }
   file<<p_str<<"\n";//write to file
}

#endif