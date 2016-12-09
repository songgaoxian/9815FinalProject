/**
 * positionservice.hpp
 * Defines the data types and Service for positions.
 *
 * @author Breman Thuraisingham
 */
#ifndef POSITION_SERVICE_HPP
#define POSITION_SERVICE_HPP

#include <string>
#include <map>
#include "soa.hpp"
#include "tradebookingservice.hpp"

using namespace std;

/**
 * Position class in a particular book.
 * Type T is the product type.
 */
template<typename T>
class Position
{

public:

  // ctor for a position
  Position(const T &_product);

  // Get the product
  const T& GetProduct() const;

  // Get the position quantity
  long GetPosition(string &book);

  // Get the aggregate position
  long GetAggregatePosition();
  //Add to positions
  void AddToPosition(long quantity, string book){
    if(positions.find(book)==positions.end()){
      //add quantity to positions
      positions.insert(make_pair(book, quantity));
    }
    else{
      positions[book]+=quantity;//update book
    }
  }

private:
  T product;
  map<string,long> positions;

};

/**
 * Position Service to manage positions across multiple books and secruties.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PositionService : public Service<string,Position <T> >
{

public:

  // Add a trade to the service
  virtual void AddTrade(const Trade<T> &trade) = 0;

};
//implement bond position service
class BondPositionService: public PositionService<Bond>
{
public:
   // Get data on our service given a key
  virtual Position<Bond>& GetData(string key){return bondPositionCache.find(key)->second;}

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Position<Bond> &data){}

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Position<Bond> > *listener){bondPositionListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<Position<Bond> >* >& GetListeners() const {return bondPositionListeners;}
  //Add a trade to the service
  virtual void AddTrade(const Trade<Bond> &trade){
    Bond bnd=trade.GetProduct();//get bond
    string bid=bnd.GetProductId();//get bond id
    string book=trade.GetBook();
    long quantity=trade.GetQuantity();
    Side side1=trade.GetSide();
    if(side1==SELL)
      quantity=-quantity;
     Position<Bond> pos(bnd);//construct position
     pos.AddToPosition(quantity,book);//update position
     map<string, Position<Bond> >::iterator thepos=bondPositionCache.find(bid);//get the position that already exists
    if(thepos==bondPositionCache.end()){
      //the product has not been registered with a position
      bondPositionCache.insert(make_pair(bid,pos));//insert position
      for(int i=0;i<bondPositionListeners.size();++i){
        //iterate listeners to add position
        bondPositionListeners[i]->ProcessAdd(pos);
      }
    }
    else{
      //the product has a position already
      thepos->second.AddToPosition(quantity,book);//update position
      for(int i=0;i<bondPositionListeners.size();++i){
        //iterate listeners to update position
        bondPositionListeners[i]->ProcessAdd(pos);
      }
    }
  }

private:
  map<string, Position<Bond> > bondPositionCache; //store position info
  vector<ServiceListener<Position<Bond> >* > bondPositionListeners;//store listeners
};

//implement BondTradeBookingServiceListener
class BondTradeListener: public ServiceListener<Trade<Bond> >
{
private:
  BondPositionService& bp_service;
public:
  //BondTradeListener(){}//constructor
  BondTradeListener(BondPositionService& service):bp_service(service){}//constructor
  // Listener callback to process adding a trade
  virtual void ProcessAdd(Trade<Bond> &data){
    bp_service.AddTrade(data);
  }

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Trade<Bond> &data){
    Side side1=data.GetSide();//get side of trade to remove
    if(side1==BUY) side1=SELL;
    else side1=BUY; //flip side
    Bond bnd=data.GetProduct();//get bond
    string tid=data.GetTradeId();//get bond id
    string book=data.GetBook();
    long quantity=data.GetQuantity();
    Trade<Bond> reverseTrade(bnd,tid,book,quantity,side1);//construct reverse trade
    bp_service.AddTrade(reverseTrade);
  }

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(Trade<Bond> &data){
  }
};



template<typename T>
Position<T>::Position(const T &_product) : product(_product){}

template<typename T>
const T& Position<T>::GetProduct() const { return product;}

template<typename T>
long Position<T>::GetPosition(string &book) { return positions[book];}

template<typename T>
long Position<T>::GetAggregatePosition()
{
  // No-op implementation - should be filled out for implementations
  long pos=0;//initialize aggregate position
  for(map<string,long>::iterator it=positions.begin();it!=positions.end();++it)
    pos+=it->second; //get aggregate position
  return pos;
}

#endif
