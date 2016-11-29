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
#include <map>
#include <unordered_map>
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
{
};

class BondPricingService: public PricingService<Bond>
{
   BondPricingService(){}
   //return best price given a key
  virtual Price<Bond>& GetData(string key){
    //declare the result pair to get all possible results
    pair<multimap<string, Price<Bond> >::iterator, multimap<string, Price<Bond> >::iterator> result;
    //initialize result
    result=priceRecord.equal_range(key);
    //declare Price to return
    Price<Bond> p;
    //my question is how to decide which price to return
    //result contains all the prices consistent with the given key
    //how to choose one price from the result
    return p;
  }
 // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Price<Bond> &data){
     //get product
     Bond product=data.GetProduct();
     //get product id
     string id=product.GetProductId();
     //record data in priceRecord
     priceRecord.insert(pair<string, Price<Bond> >(id, data));
  }

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Price<Bond> > *listener){
    price_listeners.push_back(listener);
  }

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<Price<Bond> >* >& GetListeners() const{return price_listeners;}
private:
  multimap<string, Price<Bond> > priceRecord; //keyed on product ID for price
  vector<ServiceListener<Price<Bond> >* > price_listeners;
};
//PricingServiceConnector create data in prices.txt and flow them to PricingService
class BondPricingConnector: public Connector<Price<Bond> >
{
public:
   virtual void Publish(Price<Bond> &data){}
   PricingServiceConnector(){}
   PricingServiceConnector(PricingService<Bond> & p_service, unordered_map<string, Bond> m_bond){
     //it will read from prices.txt and call OnMessage to record all prices in multimap priceRecord
   }
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

#endif
