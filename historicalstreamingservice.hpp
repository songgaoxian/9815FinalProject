/*
implement historical data servic for bond position
author: Gaoxian Song
*/
#ifndef HistoricalDataStream_HPP
#define HistoricalDataStream_HPP

#include "historicaldataservice.hpp"
#include <map>

//connector for historical data service for bond position
//paired with persist key
class BondStreamHistoricalConnector: public Connector<pair<string,PriceStream<Bond> > >
{
public:
  // Publish data to the Connector
  virtual void Publish(pair<string, PriceStream<Bond> > &data);
};
//historical dataservice for bond position
//keyed on record, not product
class BondStreamHistoricalData: public HistoricalDataService<PriceStream<Bond> >
{
private:
  int counter;//count record to determine the key
  map<string, PriceStream<Bond> > bondHistoricalCache;
  vector<ServiceListener<PriceStream<Bond> >* > bondListeners;//listeners
  BondStreamHistoricalConnector& b_historical;//connector to output file
public:
  BondStreamHistoricalData(BondStreamHistoricalConnector& src):b_historical(src){counter=1;}//constructor
  // Get data on our service given a key
  virtual PriceStream<Bond>& GetData(string key){return bondHistoricalCache.find(key)->second;}

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(PriceStream<Bond> &data){}//do nothing

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<PriceStream<Bond> > *listener){bondListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<PriceStream<Bond> >* >& GetListeners() const{return bondListeners;}
  //persist data to a store
  virtual void PersistData(string persistKey, const PriceStream<Bond>& data){
  	pair<string, PriceStream<Bond> > dataPair=make_pair(persistKey,data);//construct data pair
  	b_historical.Publish(dataPair);//publish data to file
  }
  //set key for persist data
  void SetPersistKey(PriceStream<Bond>& data){
  	//get key from counter and increment counter
  	string k=to_string(counter); ++counter;
  	PriceStream<Bond> data_copy=data; PersistData(k,data_copy);
  }
};

class BondStreamHistoricalListener: public ServiceListener<PriceStream<Bond> >
{
private:
	BondStreamHistoricalData& b_historical_data;//to flow into
public:
	BondStreamHistoricalListener(BondStreamHistoricalData& src): b_historical_data(src){}
	// Listener callback to process an update event to the Service
  virtual void ProcessUpdate(PriceStream<Bond> &data){}

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(PriceStream<Bond> &data){}

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(PriceStream<Bond> &data){b_historical_data.SetPersistKey(data);}
};

//implement publish
void BondStreamHistoricalConnector::Publish(pair<string, PriceStream<Bond> > &data1){
	ofstream file;
  file.open("./Output/Historical/streaming.txt",ios_base::app);//open the file to append
   //get key
	string k=data1.first;
	file<<k<<",";
  PriceStream<Bond> data=data1.second;//get price stream
   Bond bnd=data.GetProduct();//get prodcut of data
   string bondid=bnd.GetProductId();//get bond cusip
   file<<bondid<<",";//write cusip to file
   PriceStreamOrder bid_order=data.GetBidOrder();//get bid order
   double price=bid_order.GetPrice();//get bid price
   string p_str=PriceProcess(price);//convert to string
   file<<p_str<<",";//write to file
   long visible=bid_order.GetVisibleQuantity();//get visible qty
   file<<to_string(visible)<<",";//write to file
   long hidden=bid_order.GetHiddenQuantity();//get hidden qty
   file<<to_string(hidden)<<",";//write to file
   PriceStreamOrder offer_order=data.GetOfferOrder();//get offer order
   price=offer_order.GetPrice();//get price
   p_str=PriceProcess(price);//convert to string
   file<<p_str<<",";//write to file
   visible=offer_order.GetVisibleQuantity();//get visible qty
   file<<to_string(visible)<<",";
   hidden=offer_order.GetHiddenQuantity();
   file<<to_string(hidden)<<"\n";
}

#endif