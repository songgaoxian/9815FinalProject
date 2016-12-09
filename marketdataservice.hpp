/**
 * marketdataservice.hpp
 * Defines the data types and Service for order book market data.
 *
 * @author Breman Thuraisingham
 */
#ifndef MARKET_DATA_SERVICE_HPP
#define MARKET_DATA_SERVICE_HPP

#include <string>
#include <vector>
#include "soa.hpp"
#include <map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <cstddef>
#include <functional>
#include <cmath>


using namespace std;

// Side for market data
enum PricingSide { BID, OFFER };

/**
 * A market data order with price, quantity, and side.
 */
class Order
{

public:

  // ctor for an order
  Order(double _price, long _quantity, PricingSide _side);

  // Get the price on the order
  double GetPrice() const;

  // Get the quantity on the order
  long GetQuantity() const;

  // Get the side on the order
  PricingSide GetSide() const;

private:
  double price;
  long quantity;
  PricingSide side;

};

/**
 * Class representing a bid and offer order
 */
class BidOffer
{

public:

  // ctor for bid/offer
  BidOffer(const Order &_bidOrder, const Order &_offerOrder);

  // Get the bid order
  const Order& GetBidOrder() const;

  // Get the offer order
  const Order& GetOfferOrder() const;

private:
  Order bidOrder;
  Order offerOrder;

};

/**
 * Order book with a bid and offer stack.
 * Type T is the product type.
 */
template<typename T>
class OrderBook
{

public:

  // ctor for the order book
  OrderBook(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack);

  // Get the product
  const T& GetProduct() const;

  // Get the bid stack
  const vector<Order>& GetBidStack() const;

  // Get the offer stack
  const vector<Order>& GetOfferStack() const;

private:
  T product;
  vector<Order> bidStack;
  vector<Order> offerStack;

};

/**
 * Market Data Service which distributes market data
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class MarketDataService : public Service<string,OrderBook <T> >
{

public:

  // Get the best bid/offer order
  virtual const BidOffer GetBestBidOffer(const string &productId) = 0;

  // Aggregate the order book
  virtual const OrderBook<T> AggregateDepth(const string &productId) = 0;

};
class Key_Less{
public:
  bool operator()(double x1,double x2){
    double tol=0.000001;
    if(x1<x2 && abs(x1-x2)>tol) return true;
    return false;
  }
};
void AggregateToMap(map<double,long,Key_Less>& final_map, vector<Order>& order_stack){
  for(int i=0;i<order_stack.size();++i){
      double price=order_stack[i].GetPrice();//get price
      long qty=order_stack[i].GetQuantity();//get quantity
      if(final_map.find(price)==final_map.end()){
        final_map.insert(make_pair(price,qty));//if no such price exists, create a new entry
      }
      else{
        final_map[price]+=qty;//if such price level already exists, increment the quantity
      }
    }
}
vector<Order> GetOrderStack(map<double, long, Key_Less>& final_map, PricingSide side_){
  vector<Order> result;
  //iterate the map
   for(map<double,long,Key_Less>::iterator it=final_map.begin();it!=final_map.end();++it){
     double p=it->first; //get price
     long qty=it->second; //get quantity
     Order temp(p,qty,side_);//contruct temporary order
     result.push_back(temp);//push to result
   }
   return result;
}
//the marketdata.txt only contains the best bid and offer
class BondMarketDataService: public MarketDataService<Bond>
{
private:
  multimap<string, OrderBook<Bond> > bondMarketDataCache;
  vector< ServiceListener<OrderBook<Bond> >* > bondOrderBookListeners;
public:
  // Aggregate the order book
  virtual const OrderBook<Bond> AggregateDepth(const string &productId){
    //get the pair of iterators for bondmarketdata cache given productId
    pair<multimap<string, OrderBook<Bond> >::iterator, multimap<string, OrderBook<Bond> >::iterator> pairs=bondMarketDataCache.equal_range(productId);
    int count=0;
    //get the first orderbook
    OrderBook<Bond> orderBook1=pairs.first->second;
    Bond bnd=orderBook1.GetProduct();//get bond of the orderbook
    map<double, long, Key_Less> final_bid_price_q;//the map of bid price and quantity
    map<double, long, Key_Less> final_offer_price_q;//the map of offer price and quantity
    vector<Order> bid_stack_base=orderBook1.GetBidStack();//get bid stack of orderbook1
    vector<Order> offer_stack_base=orderBook1.GetOfferStack();//get offer stack of orderbook1
    //aggregate price and quantity pairs in maps
    AggregateToMap(final_bid_price_q,bid_stack_base);
    AggregateToMap(final_offer_price_q,offer_stack_base);
    //iterate throughout the range
    for(multimap<string, OrderBook<Bond> >::iterator it=pairs.first;it!=pairs.second;++it){
      if(count==0) {
        ++count;
      }
      else{
        OrderBook<Bond> order_book_temp=it->second;//get the current orderbook
        vector<Order> bid_stack_temp=order_book_temp.GetBidStack();//get bid stack of current orderbook
        vector<Order> offer_stack_temp=order_book_temp.GetOfferStack();//get offer stack of current orderbook
        AggregateToMap(final_bid_price_q,bid_stack_temp);//aggregate to map
        AggregateToMap(final_offer_price_q,offer_stack_temp);//aggregate to map
      }
    }
    vector<Order> bids_stack=GetOrderStack(final_bid_price_q,BID);
    vector<Order> offers_stack=GetOrderStack(final_offer_price_q,OFFER);
    //construct orderbook
    OrderBook<Bond> aggregate_book(bnd,bids_stack,offers_stack);
    bondMarketDataCache.erase(pairs.first,pairs.second);//erase previous records
    bondMarketDataCache.insert(make_pair(productId,aggregate_book));
    return aggregate_book;
  }
  
  // Get the best bid/offer order
  virtual const BidOffer GetBestBidOffer(const string &productId){
    OrderBook<Bond> o_book=AggregateDepth(productId);//get the aggregate orderbook
    vector<Order> bids=o_book.GetBidStack();//get bidstack
    vector<Order> offers=o_book.GetOfferStack();//get offerstack
    double bestbid=bids[0].GetPrice();//get price of bids[0]
    double bestoffer=offers[0].GetPrice();//get price of offers[0]
    int bidindex=0, offerindex=0;
    for(int i=1;i<bids.size();++i){
      double p=bids[i].GetPrice();//get current price
      if(p>bestbid){
        bestbid=p;//update best bid price
        bidindex=i;//update best bid index
      }
    }
    for(int i=1;i<offers.size();++i){
      double p=offers[i].GetPrice();//get current price
      if(p<bestoffer){
        bestoffer=p;
        offerindex=i;
      }
    }
    BidOffer best(bids[bidindex],offers[offerindex]);//construct best bid offer
    return best;
  }

  // Get data on our service given a key
  virtual OrderBook<Bond>& GetData(string key){
    AggregateDepth(key);//get the aggregated book
    return bondMarketDataCache.find(key)->second;
  }


  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(OrderBook<Bond> &data){
    Bond bnd=data.GetProduct();//get the bond of the data
    string bid=bnd.GetProductId();//get product id of bnd
    bondMarketDataCache.insert(make_pair(bid,data));
    //iterate listeners
    for(int i=0;i<bondOrderBookListeners.size();++i){
      bondOrderBookListeners[i]->ProcessAdd(data);//add the new data
    }
  }

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<OrderBook<Bond> > *listener){bondOrderBookListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<OrderBook<Bond> >* >& GetListeners() const{return bondOrderBookListeners;}

};
//implement connector for BondMarketDataService
class BondMarketDataConnector: public Connector<OrderBook<Bond> >
{
private:
  int counter;
public:
  BondMarketDataConnector(){counter=0;}
  // Publish data to the Connector
  virtual void Publish(OrderBook<Bond> &data){} //do nothing
  //subscribe and return subscribed data
  virtual OrderBook<Bond> Subscribe(BondMarketDataService& bmkt_data_service, map<string, Bond> m_bond){
    ifstream file;
    file.open("./Input/marketdata.txt");//open file
    string line;//store one line
    getline(file,line);//read header line
    for(int i=0;i<=counter;++i)
      getline(file,line);//read line
    ++counter;//update counter
    vector<string> infolines;//store info about readed line
    vector<string> priceparts;//store parts of prices
    boost::split(infolines,line,boost::is_any_of(","));//split line
    string bondId=infolines[0];//get bond id
    string bidstr=infolines[1];//get bid string
    double spd=1.0/128.0;//specify spread
    boost::split(priceparts,bidstr,boost::is_any_of("-"));//split bidstr
    string bid1=priceparts[0];//get part 1 of bid
    string bid2=priceparts[1];//get part 2 of bid
    double bid1num=0;//get bid num
    for(int i=0;i<bid1.length();++i){
      int current=int(bid1[i]-'0');//get current digit
      bid1num+=current*pow(10,bid1.length()-i-1);//get first part of bid
    }
    double bid2num=10.0*int(bid2[0]-'0')+int(bid2[1]-'0');//get bid2num
    bid2num=bid2num/32.0;
    double bid3num=int(bid2[2]-'0')/256.0;//get part 3 of bid
    double bestbid=bid1num+bid2num+bid3num;//sum up to get best bid
    double bestoffer=bestbid+spd;//get best offer
    long size1=10*1000000; //top level size
    vector<Order> bidStack;
    vector<Order> offerStack;
    for(int i=0;i<5;++i){
      double pricebid=bestbid-double(i)*1.0/256.0;//get the price of ith bid
      double priceoffer=bestoffer+double(i)*1.0/256.0; //get price of ith offer
      long thesize=size1*(i+1);//get the size for ith level
      Order bidOrder1(pricebid,thesize,BID);//contruct bidorder
      Order offerOrder1(priceoffer,thesize,OFFER);//construct offer order
      //fill in bid and offer stack
      bidStack.push_back(bidOrder1);
      offerStack.push_back(offerOrder1);
    }
    Bond bnd=m_bond[bondId];//get bond
    OrderBook<Bond> result(bnd,bidStack,offerStack);//construct orderbook
    bmkt_data_service.OnMessage(result);//flow into service
    return result;
  }
};


Order::Order(double _price, long _quantity, PricingSide _side)
{
  price = _price;
  quantity = _quantity;
  side = _side;
}

double Order::GetPrice() const
{
  return price;
}
 
long Order::GetQuantity() const
{
  return quantity;
}
 
PricingSide Order::GetSide() const
{
  return side;
}

BidOffer::BidOffer(const Order &_bidOrder, const Order &_offerOrder) :
  bidOrder(_bidOrder), offerOrder(_offerOrder)
{
}

const Order& BidOffer::GetBidOrder() const
{
  return bidOrder;
}

const Order& BidOffer::GetOfferOrder() const
{
  return offerOrder;
}

template<typename T>
OrderBook<T>::OrderBook(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack) :
  product(_product), bidStack(_bidStack), offerStack(_offerStack)
{
}

template<typename T>
const T& OrderBook<T>::GetProduct() const
{
  return product;
}

template<typename T>
const vector<Order>& OrderBook<T>::GetBidStack() const
{
  return bidStack;
}

template<typename T>
const vector<Order>& OrderBook<T>::GetOfferStack() const
{
  return offerStack;
}

#endif
