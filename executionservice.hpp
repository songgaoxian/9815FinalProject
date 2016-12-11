/**
 * executionservice.hpp
 * Defines the data types and Service for executions.
 *
 * @author Breman Thuraisingham
 */
#ifndef EXECUTION_SERVICE_HPP
#define EXECUTION_SERVICE_HPP

#include <string>
#include <stdlib.h>
#include "soa.hpp"
#include "marketdataservice.hpp"
#include <fstream>

enum OrderType { FOK, IOC, MARKET, LIMIT, STOP };

enum Market { BROKERTEC, ESPEED, CME };

/**
 * An execution order that can be placed on an exchange.
 * Type T is the product type.
 */
template<typename T>
class ExecutionOrder
{

public:

  // ctor for an order
  ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, long _visibleQuantity, long _hiddenQuantity, string _parentOrderId, bool _isChildOrder);

  // Get the product
  const T& GetProduct() const;

  // Get the order ID
  const string& GetOrderId() const;

  // Get the order type on this order
  OrderType GetOrderType() const;

  // Get the price on this order
  double GetPrice() const;

  // Get the visible quantity on this order
  long GetVisibleQuantity() const;

  // Get the hidden quantity
  long GetHiddenQuantity() const;

  // Get the parent order ID
  const string& GetParentOrderId() const;

  // Is child order?
  bool IsChildOrder() const;

  //Get pricingside
  PricingSide GetSide() const{return side;}

private:
  T product;
  PricingSide side;
  string orderId;
  OrderType orderType;
  double price;
  long visibleQuantity;
  long hiddenQuantity;
  string parentOrderId;
  bool isChildOrder;

};

//algo execution service
//T is product type
template<typename T>
class AlgoExecution
{
private:
  ExecutionOrder<T>& exe_orders; //each algo execution is associated with a vector of references
public:
  //constructor
  AlgoExecution(ExecutionOrder<T>& m_exe_order): exe_orders(m_exe_order){}
  //get execution order
  ExecutionOrder<T>& GetExecutionOrder(){return exe_orders;}
  //set execution order
  void SetExecutionOrder(const ExecutionOrder<T>& src){exe_orders=src;}
};

/**
 * Service for executing orders on an exchange.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class ExecutionService : public Service<string,ExecutionOrder <T> >
{

public:

  // Execute an order on a market
  virtual void ExecuteOrder(const ExecutionOrder<T>& order, Market market) = 0;

};
/*
AlgoExecutionService to execute algo
keyed on product identifier and T is product type
*/
template<typename T>
class AlgoExecutionService: public Service<string, AlgoExecution<T> >
{
public:
  virtual void ExecuteAlgo(OrderBook<T>& o_book)=0;
};
//bond algo execution service
class BondAlgoExecutionService: public AlgoExecutionService<Bond>
{
private:
  //record the most recent executed algoexecution
  map<string, AlgoExecution<Bond> > bondAlgoExeCache;
  vector< ServiceListener<AlgoExecution<Bond> >* > algoExeListeners;
  map<string, bool> isBuy;//constrol alternation
  int orderNum;//it will be converted to order id
public:
  BondAlgoExecutionService(){orderNum=1;}
   // Get data on our service given a key
  virtual AlgoExecution<Bond>& GetData(string key){
    return bondAlgoExeCache.find(key)->second;
  }

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(AlgoExecution<Bond> &data){}

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<AlgoExecution<Bond> > *listener){algoExeListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<AlgoExecution<Bond> >* >& GetListeners() const {return algoExeListeners;}
  //execute algo
  virtual void ExecuteAlgo(OrderBook<Bond>& o_book);
};
//orderbook listeners
//link orderbook from marketdata service to bondalgoexecution service
class BondMarketDataListeners: public ServiceListener<OrderBook<Bond> >
{
private:
  BondAlgoExecutionService& bnd_algo_exe;
public:
  BondMarketDataListeners(BondAlgoExecutionService& src):bnd_algo_exe(src){}
   // Listener callback to process an add event to the Service
  virtual void ProcessUpdate(OrderBook<Bond> &data){bnd_algo_exe.ExecuteAlgo(data);}

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(OrderBook<Bond> &data){}

  // Listener callback to process an update event to the Service
  virtual void ProcessAdd(OrderBook<Bond> &data){}
};

class BondExecutionConnector: public Connector<pair<Market, ExecutionOrder<Bond> > >
{
public:
   virtual void Publish(pair<Market, ExecutionOrder<Bond> > &data);
};

//implement bondexecution service
class BondExecutionService: public ExecutionService<Bond>
{
private:
  map<string, ExecutionOrder<Bond> > bondExeOrderCache;//record most recent executed order
  //this vector should only contain one listener
  vector< ServiceListener<ExecutionOrder<Bond> >* > exeOrderListeners;
  BondExecutionConnector b_exe_connector;
public:
   // Get data on our service given a key
  virtual ExecutionOrder<Bond>& GetData(string key){
    return bondExeOrderCache.find(key)->second;
  }

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(ExecutionOrder<Bond> &data){}//do nothing

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<ExecutionOrder<Bond> > *listener){exeOrderListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<ExecutionOrder<Bond> >* >& GetListeners() const {return exeOrderListeners;}

  virtual void ExecuteOrder(const ExecutionOrder<Bond>& order, Market market);
};

//algoexecution listener, link to bondexecutionservice
class BondAlgoExecutionListener: public ServiceListener<AlgoExecution<Bond> >
{
private:
  BondExecutionService& b_exe_service;
public:
  BondAlgoExecutionListener(BondExecutionService& src):b_exe_service(src){}//constructor
   // Listener callback to process an add event to the Service
  virtual void ProcessUpdate(AlgoExecution<Bond> &data){}

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(AlgoExecution<Bond> &data){}

  // Listener callback to process an update event to the Service
  virtual void ProcessAdd(AlgoExecution<Bond> &data);
};

template<typename T>
ExecutionOrder<T>::ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, long _visibleQuantity, long _hiddenQuantity, string _parentOrderId, bool _isChildOrder) :
  product(_product)
{
  side = _side;
  orderId = _orderId;
  orderType = _orderType;
  price = _price;
  visibleQuantity = _visibleQuantity;
  hiddenQuantity = _hiddenQuantity;
  parentOrderId = _parentOrderId;
  isChildOrder = _isChildOrder;
}

template<typename T>
const T& ExecutionOrder<T>::GetProduct() const
{
  return product;
}

template<typename T>
const string& ExecutionOrder<T>::GetOrderId() const
{
  return orderId;
}

template<typename T>
OrderType ExecutionOrder<T>::GetOrderType() const
{
  return orderType;
}

template<typename T>
double ExecutionOrder<T>::GetPrice() const
{
  return price;
}

template<typename T>
long ExecutionOrder<T>::GetVisibleQuantity() const
{
  return visibleQuantity;
}

template<typename T>
long ExecutionOrder<T>::GetHiddenQuantity() const
{
  return hiddenQuantity;
}

template<typename T>
const string& ExecutionOrder<T>::GetParentOrderId() const
{
  return parentOrderId;
}

template<typename T>
bool ExecutionOrder<T>::IsChildOrder() const
{
  return isChildOrder;
}

void BondAlgoExecutionService::ExecuteAlgo(OrderBook<Bond>& o_book){
     Bond bnd=o_book.GetProduct();//get product of bond
     string bid=bnd.GetProductId();//get bond id
     //initially set the order to buy, otherwise alternate the isbuy signal for the product
     if(isBuy.find(bid)==isBuy.end()){isBuy.insert(make_pair(bid,true));}
     else{isBuy[bid]=!isBuy[bid];}
     //assume each execution order always sweep the entire best bid or best offer of market
     if(isBuy[bid]){
      //if we need to buy
      vector<Order> offers=o_book.GetOfferStack();
      vector<Order>::iterator index=offers.begin();//point to best offer 
      double p=offers[0].GetPrice();//store best offer price
      vector<Order>::iterator it=offers.begin();
      for(it=offers.begin()+1;it<offers.end();++it){
        if(it->GetPrice()<p){
          //find better offer
          p=it->GetPrice();//update price
          index=it;//update index
        }
      }
      //get quantity of the best offer
      long q=index->GetQuantity();
      long visible=q*0.3;
      long invisible=q-visible;
      offers.erase(index); //this order of market is exhausted
      o_book.SetOfferStack(offers);//update offerstack
      //construct the execution order
      ExecutionOrder<Bond> e_order(bnd, BID, to_string(orderNum),MARKET,p,visible,invisible,to_string(orderNum),false);
      orderNum++;
      AlgoExecution<Bond> algo_exe(e_order);//construct algo execution object
      map<string, AlgoExecution<Bond> >::iterator m_algo_exe=bondAlgoExeCache.find(bid);
      if(m_algo_exe==bondAlgoExeCache.end()){
        //new order
        bondAlgoExeCache.insert(make_pair(bid,algo_exe));
        //iterate listeners
        for(int i=0;i<algoExeListeners.size();++i){
          algoExeListeners[i]->ProcessAdd(algo_exe);
        }
      }
      else{
        //exist past order
        bondAlgoExeCache.erase(bid);
        bondAlgoExeCache.insert(make_pair(bid,algo_exe));
        //iterate listeners
        for(int i=0;i<algoExeListeners.size();++i){
          algoExeListeners[i]->ProcessAdd(algo_exe);
        }
      }
     }
     else{
     //if we need to sell
      vector<Order> bids=o_book.GetBidStack();
      vector<Order>::iterator index=bids.begin();//point to best bid 
      double p=bids[0].GetPrice();//store best bid price
      vector<Order>::iterator it=bids.begin();
      for(it=bids.begin()+1;it<bids.end();++it){
        if(it->GetPrice()>p){
          //find better bid
          p=it->GetPrice();//update price
          index=it;//update index
        }
      }
      //get quantity of the best bid
      long q=index->GetQuantity();
      long visible=q*0.3;
      long invisible=q-visible;
      bids.erase(index); //this order of market is exhausted
      o_book.SetBidStack(bids);//update bidstack
      //construct the execution order
      ExecutionOrder<Bond> e_order(bnd, OFFER, to_string(orderNum),MARKET,p,visible,invisible,to_string(orderNum),false);
      orderNum++;
      AlgoExecution<Bond> algo_exe(e_order);//construct algo execution object
      map<string, AlgoExecution<Bond> >::iterator m_algo_exe=bondAlgoExeCache.find(bid);
      if(m_algo_exe==bondAlgoExeCache.end()){
        //new order
        bondAlgoExeCache.insert(make_pair(bid,algo_exe));
        //iterate listeners
        for(int i=0;i<algoExeListeners.size();++i){
          algoExeListeners[i]->ProcessAdd(algo_exe);
        }
      }
      else{
        //exist past order
        bondAlgoExeCache.erase(bid);
        bondAlgoExeCache.insert(make_pair(bid,algo_exe));
        //iterate listeners
        for(int i=0;i<algoExeListeners.size();++i){
          algoExeListeners[i]->ProcessAdd(algo_exe);
        }
      }
    }
  }

void BondExecutionService::ExecuteOrder(const ExecutionOrder<Bond>& order, Market market){
    Bond bnd=order.GetProduct();//get product of the order
    string bondid=bnd.GetProductId();//get product id
    map<string, ExecutionOrder<Bond> >::iterator it=bondExeOrderCache.find(bondid);//get corresponding entry
    ExecutionOrder<Bond> copy=order;
    if(it==bondExeOrderCache.end()){
      //new entry
      bondExeOrderCache.insert(make_pair(bondid,order));//insert into cache
      for(int i=0;i<exeOrderListeners.size();++i){
        //iterate listeners
         exeOrderListeners[i]->ProcessAdd(copy);
      }
    }
    else{
      bondExeOrderCache.erase(bondid);//remove old entry
      bondExeOrderCache.insert(make_pair(bondid,order));//insert into cache
      for(int i=0;i<exeOrderListeners.size();++i){
        exeOrderListeners[i]->ProcessUpdate(copy);//iterate listeners
      }
    }
    pair<Market, ExecutionOrder<Bond> > currentOrder(make_pair(market,copy));
    b_exe_connector.Publish(currentOrder);
  }

  void BondAlgoExecutionListener::ProcessAdd(AlgoExecution<Bond> &data){
    ExecutionOrder<Bond> exe_order=data.GetExecutionOrder();//get the executionorder of data
    int i=rand()%3;//to determine the market
    Market mkt;
    //assign mkt randomly based on i
    switch(i){
      case 0: mkt=BROKERTEC;
              break;
      case 1: mkt=ESPEED;
              break;
      case 2: mkt=CME;
              break;
    }
    b_exe_service.ExecuteOrder(exe_order,mkt);//flow data to execution service
  }

void BondExecutionConnector::Publish(pair<Market, ExecutionOrder<Bond> > &data){
   ofstream file;
   file.open("./Output/ExecutionOrders.txt",ios_base::app);//open the file to append
   Market mkt=data.first;//get the market
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
   //write corresponding market to file
   switch(mkt){
    case BROKERTEC: file<<"BROKERTEC,";
                    break;
    case ESPEED: file<<"ESPEED,";
                 break;
    case CME: file<<"CME,";
              break;
   }
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
