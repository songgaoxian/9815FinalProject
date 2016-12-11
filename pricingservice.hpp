/**
 * pricingservice.hpp
 * Defines the data types and Service for internal prices.
 *
 * @author Breman Thuraisingham
 */
#ifndef PRICING_SERVICE_HPP
#define PRICING_SERVICE_HPP

#include <string>
#include "soa.hpp"
#include "products.hpp"
#include <map>
#include "tradebookingservice.hpp"
#include <algorithm>

/**
 * A price object consisting of mid and bid/offer spread.
 * Type T is the product type.
 */
template<typename T>
class Price
{

public:
  // ctor for a price
  Price(const T &_product, double _mid, double _bidOfferSpread);

  // Get the product
  const T& GetProduct() const;

  // Get the mid price
  double GetMid() const;

  // Get the bid/offer spread around the mid
  double GetBidOfferSpread() const;

private:
  const T& product;
  double mid;
  double bidOfferSpread;
};

/**
 * Pricing Service managing mid prices and bid/offers.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PricingService : public Service<string,Price <T> >
{};

//specify bondpriceservice
class BondPriceService: public PricingService<Bond>
{
public:
  BondPriceService(){}//constructor
   // Get data on our service given a key
  virtual Price<Bond>& GetData(string key){return bondPriceCache.find(key)->second;}

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Price<Bond> &data);
  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Price<Bond> > *listener){bondPriceListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<Price<Bond> >* >& GetListeners() const{return bondPriceListeners;}

private:
  map<string, Price<Bond> > bondPriceCache;//store price records
  vector<ServiceListener<Price<Bond> >* > bondPriceListeners;  
};

//specify connector for pricing service
class BondPriceConnector: public Connector<Price<Bond> >
{
private:
  int counter;//get counter
public:
  //constructor
  BondPriceConnector(){counter=0;}
  // Publish data to the Connector
  virtual void Publish(Price<Bond> &data){}//do nothing
  //subscribe and return subscribed data
  virtual Price<Bond> Subscribe(BondPriceService& bprice_service, map<string, Bond> m_bond);
};

template<typename T>
Price<T>::Price(const T &_product, double _mid, double _bidOfferSpread) :
  product(_product)
{
  mid = _mid;
  bidOfferSpread = _bidOfferSpread;
}

template<typename T>
const T& Price<T>::GetProduct() const
{
  return product;
}

template<typename T>
double Price<T>::GetMid() const
{
  return mid;
}

template<typename T>
double Price<T>::GetBidOfferSpread() const
{
  return bidOfferSpread;
}

void BondPriceService::OnMessage(Price<Bond> &data){
    //get bond
    Bond bnd=data.GetProduct();
    //get bond id
    string bndid=bnd.GetProductId();
    if(bondPriceCache.find(bndid)==bondPriceCache.end()){
      //insert data to cache
      bondPriceCache.insert(make_pair(bndid,data));
      //use listeners to add
      for(int i=0;i<bondPriceListeners.size();++i)
        bondPriceListeners[i]->ProcessAdd(data);
    }
    else{
      bondPriceCache.erase(bndid);//erase bndid
      //update data
      bondPriceCache.insert(make_pair(bndid,data));
      //use listeners to update
      for(int i=0;i<bondPriceListeners.size();++i)
        bondPriceListeners[i]->ProcessUpdate(data);
    }
  }

Price<Bond> BondPriceConnector::Subscribe(BondPriceService& bprice_service, map<string, Bond> m_bond){
    ifstream file;
    file.open("./Input/prices.txt");//open file
    string line;//store one line
    getline(file,line);//read header line
    for(int i=0;i<=counter;++i)
      getline(file,line);//read line
    ++counter;//update counter
    vector<string> tradelines;//store info about readed line
    vector<string> priceparts;//store parts of prices
    boost::split(tradelines,line,boost::is_any_of(","));//split line
    string bondId=tradelines[0];//get bond id
    string bidstr=tradelines[1];//get bid string
    string spreadstr=tradelines[3];//get spread string
    double spd=int(spreadstr[0]-'0')*1.0/256.0;//convert to double
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
    double bidtotal=bid1num+bid2num+bid3num;//sum up bid
    double mid=bidtotal+0.5*spd; //get mid price
    Bond bnd=m_bond[bondId];//get bond
    Price<Bond> p_bond(bnd,mid,spd);//construct price
    bprice_service.OnMessage(p_bond);//flow data to service
    return p_bond;
  }
#endif
