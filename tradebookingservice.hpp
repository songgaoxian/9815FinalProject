/**
 * tradebookingservice.hpp
 * Defines the data types and Service for trade booking.
 *
 * @author Breman Thuraisingham
 */
#ifndef TRADE_BOOKING_SERVICE_HPP
#define TRADE_BOOKING_SERVICE_HPP

#include <string>
#include <vector>
#include "soa.hpp"
#include "products.hpp"
#include <map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <cstddef>

// Trade sides
enum Side { BUY, SELL };

/**
 * Trade object with a price, side, and quantity on a particular book.
 * Type T is the product type.
 */
template<typename T>
class Trade
{

public:

  // ctor for a trade
  Trade(const T &_product, string _tradeId, string _book, long _quantity, Side _side);

  // Get the product
  const T& GetProduct() const;

  // Get the trade ID
  const string& GetTradeId() const;

  // Get the book
  const string& GetBook() const;

  // Get the quantity
  long GetQuantity() const;
  
  // Get the side
  Side GetSide() const;

private:
  T product;
  string tradeId;
  string book;
  long quantity;
  Side side;

};

/**
 * Trade Booking Service to book trades to a particular book.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class TradeBookingService : public Service<string,Trade <T> >
{
public:
  // Book the trade
  virtual void BookTrade(const Trade<T> &trade)=0;
};

//implement bondtradebookservice
class BondTradeBookService: public TradeBookingService<Bond>
{
public:
  BondTradeBookService(){}
   // Get data on our service given a key
  virtual Trade<Bond>& GetData(string key){
    return bondBookCache.find(key)->second;
  }

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Trade<Bond> &data){
    string tid=data.GetTradeId(); //get trade id
    Trade<Bond> tradeCopy=data;
    map<string, Trade<Bond> >::iterator ptr_trade=bondBookCache.find(tid);//try to find tid
    if(ptr_trade!=bondBookCache.end()){
      for(int i=0;i<bondTradeListers.size();++i){
        bondTradeListers[i]->ProcessRemove(ptr_trade->second); //remove old trade
        bondTradeListers[i]->ProcessAdd(tradeCopy);//update this trade
      }
      bondBookCache.erase(tid);
      bondBookCache.insert(make_pair(tid,data)); //store data in cache
    }
    else{
      bondBookCache.insert(make_pair(tid,data));
      //iterate service listeners
      for(int i=0;i<bondTradeListers.size();++i)
        bondTradeListers[i]->ProcessAdd(tradeCopy); //invoke listeners
    }
  }

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Trade<Bond> > *listener){bondTradeListers.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<Trade<Bond> >* >& GetListeners() const{return bondTradeListers;}

  //book trade
  virtual void BookTrade(const Trade<Bond> &trade){
    Trade<Bond> tradeCopy=trade; //get a copy of trade
    string tid=tradeCopy.GetTradeId();//get trade id
    map<string, Trade<Bond> >::iterator ptr_trade=bondBookCache.find(tid);
    if(ptr_trade==bondBookCache.end()){
      //iterate service listeners
      for(int i=0;i<bondTradeListers.size();++i)
        bondTradeListers[i]->ProcessAdd(tradeCopy); //invoke listeners
      bondBookCache.insert(std::make_pair(tid,tradeCopy));
    }
    else{
      //iterate service listeners
      for(int i=0;i<bondTradeListers.size();++i){
        bondTradeListers[i]->ProcessRemove(ptr_trade->second); //remove old trade
        bondTradeListers[i]->ProcessAdd(tradeCopy);//update this trade
      }
      bondBookCache.erase(tid);
      bondBookCache.insert(make_pair(tid,tradeCopy));//update cache
    }
  }


private:
  map<string, Trade<Bond> > bondBookCache; //store records of trade
  vector<ServiceListener<Trade<Bond> >* > bondTradeListers; //store a list of listeners
};

//implement BondTradeBookingConnector class
class BondTradeBookingConnector: public Connector<Trade<Bond> >
{
private:
  int counter;
public:
  //it is a subscribe-only connector, so publish do nothing
  virtual void Publish(Trade<Bond> &data){}
  BondTradeBookingConnector(){counter=0;}
  virtual Trade<Bond> Subscribe(BondTradeBookService& bt_book_service, map<string, Bond> m_bond){
    ifstream file;
    file.open("./Input/trades.txt");
    string line;//store one line
    vector<string> tradelines; //store info about each line
    //read header line
    getline(file,line);
    for(int i=0;i<=counter;++i)
       getline(file,line);//read line
    ++counter;//increment counter
    boost::split(tradelines,line,boost::is_any_of(",")); //split the line
    string tid=tradelines[0];//get trade id
    string pid=tradelines[1];//get product id
    string book=tradelines[2];//get trade book
    long quantity=stol(tradelines[3],nullptr);//get trade quantity
    string side_str=tradelines[4];//get side
    Side side1;
    if(side_str=="BUY"){side1=BUY;}
    else{side1=SELL;}
    //construct trade
    Trade<Bond> tbond1(m_bond[pid],tid,book,quantity,side1);
    //flow data to service via OnMessage
    bt_book_service.OnMessage(tbond1);
    return tbond1;
  }
};


template<typename T>
Trade<T>::Trade(const T &_product, string _tradeId, string _book, long _quantity, Side _side) :
  product(_product)
{
  tradeId = _tradeId;
  book = _book;
  quantity = _quantity;
  side = _side;
}

template<typename T>
const T& Trade<T>::GetProduct() const
{
  return product;
}

template<typename T>
const string& Trade<T>::GetTradeId() const
{
  return tradeId;
}

template<typename T>
const string& Trade<T>::GetBook() const
{
  return book;
}

template<typename T>
long Trade<T>::GetQuantity() const
{
  return quantity;
}

template<typename T>
Side Trade<T>::GetSide() const
{
  return side;
}

/*template<typename T>
void TradeBookingService<T>::BookTrade(const Trade<T> &trade)
{
}
*/

#endif
