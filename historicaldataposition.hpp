/*
implement historical data servic for bond position
author: Gaoxian Song
*/
#ifndef HistoricalDataPosition_HPP
#define HistoricalDataPosition_HPP

#include "historicaldataservice.hpp"
#include <map>

//connector for historical data service for bond position
//paired with persist key
class BondPositionHistoricalConnector: public Connector<pair<string, Position<Bond> > >
{
public:
  // Publish data to the Connector
  virtual void Publish(pair<string, Position<Bond> > &data);
};
//historical dataservice for bond position
//keyed on record, not product
class BondPositionHistoricalData: public HistoricalDataService<Position<Bond> >
{
private:
  int counter;//count record to determine the key
  map<string, Position<Bond> > bondHistoricalPositionCache;
  vector<ServiceListener<Position<Bond> >* > bondPositionListeners;//listeners
  BondPositionHistoricalConnector& b_pos_historical;//connector to output file
public:
  BondPositionHistoricalData(BondPositionHistoricalConnector& src):b_pos_historical(src){counter=1;}//constructor
  // Get data on our service given a key
  virtual Position<Bond>& GetData(string key){return bondHistoricalPositionCache.find(key)->second;}

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Position<Bond> &data){}//do nothing

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Position<Bond> > *listener){bondPositionListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<Position<Bond> >* >& GetListeners() const{return bondPositionListeners;}
  //persist data to a store
  virtual void PersistData(string persistKey, const Position<Bond>& data){
  	pair<string, Position<Bond> > dataPair=make_pair(persistKey,data);//construct data pair
  	b_pos_historical.Publish(dataPair);//publish data to file
  }
  //set key for persist data
  void SetPersistKey(Position<Bond>& data){
  	//get key from counter and increment counter
  	string k=to_string(counter); ++counter;
  	Position<Bond> data_copy=data; PersistData(k,data_copy);
  }
};

class BondPositionHistoricalListener: public ServiceListener<Position<Bond> >
{
private:
	BondPositionHistoricalData& b_historical_data;//to flow into
public:
	BondPositionHistoricalListener(BondPositionHistoricalData& src): b_historical_data(src){}
	// Listener callback to process an add event to the Service
  virtual void ProcessAdd(Position<Bond> &data){}

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Position<Bond> &data){}

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(Position<Bond> &data){b_historical_data.SetPersistKey(data);}
};

//implement publish
void BondPositionHistoricalConnector::Publish(pair<string, Position<Bond> > &data){
	ofstream file;
    file.open("./Output/Historical/position.txt",ios_base::app);//open the file to append
   //get key
	string k=data.first;
	file<<k<<",";
	//get position
	Position<Bond> pos=data.second;
	//get bond
	Bond bnd=pos.GetProduct();
	//get bond id
	string bondid=bnd.GetProductId();
    file<<bondid<<",";
    long aqty=pos.GetAggregatePosition();//get aggregate position
    file<<to_string(aqty)<<",";
    string book="TRSY1";
    long q1=pos.GetPosition(book);//get quatity for TRSY1
    file<<to_string(q1)<<",";
    book="TRSY2";
    long q2=pos.GetPosition(book);//get quantity for TRSY2
    file<<to_string(q2)<<",";
    book="TRSY3";
    long q3=pos.GetPosition(book);//get quantity for TRSY3
    file<<to_string(q3)<<"\n";
}

#endif