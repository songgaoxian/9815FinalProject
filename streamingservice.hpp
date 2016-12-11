/**
 * streamingservice.hpp
 * Defines the data types and Service for price streams.
 *
 * @author Breman Thuraisingham
 */
#ifndef STREAMING_SERVICE_HPP
#define STREAMING_SERVICE_HPP

#include "soa.hpp"
#include "marketdataservice.hpp"
#include "pricingservice.hpp"
#include <stdlib.h>

/**
 * A price stream order with price and quantity (visible and hidden)
 */
class PriceStreamOrder
{

public:

  // ctor for an order
  PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side);

  // The side on this order
  PricingSide GetSide() const;

  // Get the price on this order
  double GetPrice() const;

  // Get the visible quantity on this order
  long GetVisibleQuantity() const;

  // Get the hidden quantity on this order
  long GetHiddenQuantity() const;

private:
  double price;
  long visibleQuantity;
  long hiddenQuantity;
  PricingSide side;

};

/**
 * Price Stream with a two-way market.
 * Type T is the product type.
 */
template<typename T>
class PriceStream
{
public:
  // ctor
  PriceStream(const T &_product, const PriceStreamOrder &_bidOrder, const PriceStreamOrder &_offerOrder);

  // Get the product
  const T& GetProduct() const;

  // Get the bid order
  const PriceStreamOrder& GetBidOrder() const;

  // Get the offer order
  const PriceStreamOrder& GetOfferOrder() const;

private:
  T product;
  PriceStreamOrder bidOrder;
  PriceStreamOrder offerOrder;

};

template<typename T>
class AlgoStream{
private:
  PriceStream<T>& p_stream;
public:
  AlgoStream(PriceStream<T>& src):p_stream(src){}//constructor
  PriceStream<T>& GetPrcieStream(){return p_stream;}//getter
  void SetPriceStream(PriceStream<T>& src){p_stream=src;}//setter
};

/**
 * Streaming service to publish two-way prices.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class StreamingService : public Service<string,PriceStream <T> >
{

public:

  // Publish two-way prices
  virtual void PublishPrice(const PriceStream<T>& priceStream) = 0;

};

/*
algo streaming service to link with bond pricing service 
keyed on product identifier
*/
template<typename T>
class AlgoStreamingService: public Service<string, AlgoStream<T> >
{
public:
  //invoked by listeners to flow in data
  virtual void ExecuteAlgoStream(AlgoStream<T>& data)=0;
};

class BondAlgoStreamingService: public AlgoStreamingService<Bond>
{
private:
  map<string,AlgoStream<Bond> > bondAlgoStreamCache;
  vector< ServiceListener<AlgoStream<Bond> >* > algoStreamListeners;
public:
   // Get data on our service given a key
  virtual AlgoStream<Bond>& GetData(string key){
    return bondAlgoStreamCache.find(key)->second;
  }

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(AlgoStream<Bond> &data){}//do nothing

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<AlgoStream<Bond> > *listener){algoStreamListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<AlgoStream<Bond> >* >& GetListeners() const {return algoStreamListeners;}

  virtual void ExecuteAlgoStream(AlgoStream<Bond>& data);
};

class BondPriceListener: public ServiceListener<Price<Bond> >
{
private:
  BondAlgoStreamingService& b_algo_stream;
public:
  BondPriceListener(BondAlgoStreamingService& src): b_algo_stream(src){}//constructor
   // Listener callback to process an add event to the Service
  virtual void ProcessUpdate(Price<Bond> &data){}

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Price<Bond> &data){}

  // Listener callback to process an update event to the Service
  virtual void ProcessAdd(Price<Bond> &data);
};
//publish pricestream
class BondStreamingConnector: public Connector<PriceStream<Bond> >
{
public:
  virtual void Publish(PriceStream<Bond> &data);
};

class BondStreamingService: public StreamingService<Bond>
{
private:
  map<string, PriceStream<Bond> > bondPriceStreamCache;
  vector<ServiceListener<PriceStream<Bond> >* > priceStreamListeners;
  BondStreamingConnector b_stream_connector;
public:
   // Get data on our service given a key
  virtual PriceStream<Bond>& GetData(string key){
    return bondPriceStreamCache.find(key)->second;
  }

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(PriceStream<Bond> &data){}//do nothing

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<PriceStream<Bond> > *listener){priceStreamListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<PriceStream<Bond> >* >& GetListeners() const {return priceStreamListeners;}

  // Publish two-way prices
  void PublishPrice(const PriceStream<Bond>& priceStream);
};

class BondAlgoStreamListener: public ServiceListener<AlgoStream<Bond> >
{
private:
  BondStreamingService& b_stream_service;
public:
  BondAlgoStreamListener(BondStreamingService& src): b_stream_service(src){}//constructor
   // Listener callback to process an add event to the Service
  virtual void ProcessUpdate(AlgoStream<Bond> &data){}

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(AlgoStream<Bond> &data){}

  // Listener callback to process an update event to the Service
  virtual void ProcessAdd(AlgoStream<Bond> &data){
    PriceStream<Bond> p_stream=data.GetPrcieStream();//get the price stream
    b_stream_service.PublishPrice(p_stream);//publish the stream
  }
};

PriceStreamOrder::PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side)
{
  price = _price;
  visibleQuantity = _visibleQuantity;
  hiddenQuantity = _hiddenQuantity;
  side = _side;
}

double PriceStreamOrder::GetPrice() const
{
  return price;
}

long PriceStreamOrder::GetVisibleQuantity() const
{
  return visibleQuantity;
}

long PriceStreamOrder::GetHiddenQuantity() const
{
  return hiddenQuantity;
}

template<typename T>
PriceStream<T>::PriceStream(const T &_product, const PriceStreamOrder &_bidOrder, const PriceStreamOrder &_offerOrder) :
  product(_product), bidOrder(_bidOrder), offerOrder(_offerOrder)
{
}

template<typename T>
const T& PriceStream<T>::GetProduct() const
{
  return product;
}

template<typename T>
const PriceStreamOrder& PriceStream<T>::GetBidOrder() const
{
  return bidOrder;
}

template<typename T>
const PriceStreamOrder& PriceStream<T>::GetOfferOrder() const
{
  return offerOrder;
}

void BondAlgoStreamingService::ExecuteAlgoStream(AlgoStream<Bond>& data){
  PriceStream<Bond> p_stream=data.GetPrcieStream();//get the pricestream associated
  Bond bnd=p_stream.GetProduct();//get the bond
  string bondid=bnd.GetProductId();//get the bond id
  map<string, AlgoStream<Bond> >::iterator it=bondAlgoStreamCache.find(bondid);//locate the bond
  if(it==bondAlgoStreamCache.end()){
    //new entry
    bondAlgoStreamCache.insert(make_pair(bondid,data));
  }
  else{
    //erase previous entry
    bondAlgoStreamCache.erase(bondid);
    bondAlgoStreamCache.insert(make_pair(bondid,data));//insert new entry
  }
  for(int i=0;i<algoStreamListeners.size();++i){
    algoStreamListeners[i]->ProcessAdd(data);//invoke listeners for new data addition
  }
}

void BondPriceListener::ProcessAdd(Price<Bond>& data){
 Bond bnd=data.GetProduct();//get the corresponding bond
 string bondid=bnd.GetProductId();//get bond id
 double mid=data.GetMid();//get mid price
 double spread=data.GetBidOfferSpread();//get spread
 double bidprice=mid-0.5*spread;//get bid price
 double offerprice=mid+0.5*spread;//get offer price
 long visible=(rand()%10+1)*10000;//set random visible quantity
 long hidden=(rand()%20+1)*15000;//set random hidden quantity
 PriceStreamOrder bid_order(bidprice,visible,hidden,BID);//construct bid order
 visible=(rand()%10+1)*10000;//set random visible qty
 hidden=(rand()%20+1)*15000;//set random hidden qty
 PriceStreamOrder offer_order(offerprice,visible,hidden,OFFER);//construct offer order
 PriceStream<Bond> p_stream(bnd,bid_order,offer_order);//construct pricestream
 AlgoStream<Bond> algo_stream(p_stream);//construct algo stream
 b_algo_stream.ExecuteAlgoStream(algo_stream);//flow into service
}

void BondStreamingService::PublishPrice(const PriceStream<Bond>& priceStream){
  Bond bnd=priceStream.GetProduct();//get bond
  string bondid=bnd.GetProductId();//get bond id
  PriceStream<Bond> copy=priceStream;
  map<string, PriceStream<Bond> >::iterator it=bondPriceStreamCache.find(bondid);//find entry
  if(it==bondPriceStreamCache.end()){
    //new entry
    bondPriceStreamCache.insert(make_pair(bondid,copy));
  }
  else{
    //exist old entry
    bondPriceStreamCache.erase(bondid);
    bondPriceStreamCache.insert(make_pair(bondid,copy));//insert new entry
  }
  for(int i=0;i<priceStreamListeners.size();++i){
    //iterate listeners to add new data
    priceStreamListeners[i]->ProcessAdd(copy);
  }
  //write to file through connector
  b_stream_connector.Publish(copy);
}

//convert double price to suitable string form
string PriceProcess(double p){
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
   return p_str;
}

void BondStreamingConnector::Publish(PriceStream<Bond>& data){
   ofstream file;
   file.open("./Output/PriceStreams.txt",ios_base::app);//open the file to append
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
