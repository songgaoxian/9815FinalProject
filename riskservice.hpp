/**
 * riskservice.hpp
 * Defines the data types and Service for fixed income risk.
 *
 * @author Breman Thuraisingham
 */
#ifndef RISK_SERVICE_HPP
#define RISK_SERVICE_HPP

#include "soa.hpp"
#include "positionservice.hpp"

/**
 * PV01 risk.
 * Type T is the product type.
 */
template<typename T>
class PV01
{

public:

  // ctor for a PV01 value
  PV01(const T &_product, double _pv01, long _quantity);

  // Get the product on this PV01 value
  const T& GetProduct() const{return product;}

  // Get the PV01 value
  double GetPV01() const{return pv01;}
  //update pv01 value
  void SetPV01(double pv01_){pv01=pv01_;}

  // Get the quantity that this risk value is associated with
  long GetQuantity() const{return quantity;}

  //update quantity
  void AddQuantity(long q){quantity+=q;}

private:
  T product;
  double pv01;
  long quantity;

};

/**
 * A bucket sector to bucket a group of securities.
 * We can then aggregate bucketed risk to this bucket.
 * Type T is the product type.
 */
template<typename T>
class BucketedSector
{

public:

  // ctor for a bucket sector
  BucketedSector(const vector<T> &_products, string _name);

  // Get the products associated with this bucket
  const vector<T>& GetProducts() const;

  // Get the name of the bucket
  const string& GetName() const;

private:
  vector<T> products;
  string name;

};

/**
 * Risk Service to vend out risk for a particular security and across a risk bucketed sector.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class RiskService : public Service<string,PV01 <T> >
{

public:

  // Add a position that the service will risk
  virtual void AddPosition(Position<T> &position) = 0;

  // Get the bucketed risk for the bucket sector
  //const PV01<T>& GetBucketedRisk(const BucketedSector<T> &sector) const = 0;
  virtual double GetBucketedRisk(const BucketedSector<T> &sector) const=0;

};
//implement risk service for bond
class BondRiskService: public RiskService<Bond>
{
private:
  map<string, PV01<Bond> > bondRiskCache; //keep a local record for pv01
  vector<ServiceListener<PV01<Bond> >* > bondRiskListeners;
  map<string, double> bondPV01;//map each bond to one pv01 value
public:
  //bondPV01_ must contain all 6 bonds pv01 info
  BondRiskService(map<string,double>& bondPV01_):bondPV01(bondPV01_){}
  void UpdateBondPV01(string bondid, double newpv01){
    //assume bondPV01 always contain all bonds'pv01 info
    bondPV01[bondid]=newpv01;
    //get the corresponding record in cache
    map<string,PV01<Bond> >::iterator the_pv01=bondRiskCache.find(bondid);
    if(the_pv01!=bondRiskCache.end()){
      //only when the corresponding cache exists for the bond, it is necessary to update
      the_pv01->second.SetPV01(newpv01);
      for(int i=0;i<bondRiskListeners.size();++i){
        //update
        bondRiskListeners[i]->ProcessUpdate(the_pv01->second);//use update when value of pv01 changes
      }
    }
  }
   // Get data on our service given a key
  virtual PV01<Bond>& GetData(string key){return bondRiskCache.find(key)->second;}

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(PV01<Bond> &data){}//do nothing as no need for connector

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<PV01<Bond> > *listener){bondRiskListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<PV01<Bond> >* >& GetListeners() const{return bondRiskListeners;}
  // Add a position that the service will risk
  virtual void AddPosition(Position<Bond> &position){
    Bond bnd=position.GetProduct();//get bond of the position
    string rid=bnd.GetProductId();//get product id of the bond
    double pv_01=bondPV01[rid];//get pv01 value of the bond
    long quantity=position.GetAggregatePosition();//get the
    PV01<Bond> pv01_bnd(bnd,pv_01,quantity);//construct the pv01 to add
    map<string, PV01<Bond> >::iterator the_pv01=bondRiskCache.find(rid);//get the pv01 already exists
    if(the_pv01==bondRiskCache.end()){
      //new product entry
      bondRiskCache.insert(make_pair(rid,pv01_bnd));//insert into cache
      for(int i=0;i<bondRiskListeners.size();++i){
        bondRiskListeners[i]->ProcessAdd(pv01_bnd);//invoke listeners for add
      }
    }
    else{
      //the pv01 already existed
      the_pv01->second.AddQuantity(quantity);//update quantity
      for(int i=0;i<bondRiskListeners.size();++i){
        bondRiskListeners[i]->ProcessAdd(pv01_bnd);//invoke listeners for add
      }
    }
  }

  // Get the bucketed risk for the bucket sector
  virtual double GetBucketedRisk(const BucketedSector<Bond> &sector) const{
    //assume all bonds in sector has pv01 record in cache
    vector<Bond> bonds=sector.GetProducts();
    double risk_bucket=0;
    for(int i=0;i<bonds.size();++i){
      //iterate bonds
      Bond bnd=bonds[i];//get bond
      string bid=bnd.GetProductId();//get bond id
      PV01<Bond> thepv01=bondRiskCache.find(bid)->second;//get the pv01 of this bond
      long q=thepv01.GetQuantity();//get quantity of the associated pv01
      risk_bucket+=double(q)*thepv01.GetPV01();//get accumulate risk of the bucket
    }
    return risk_bucket;
  }
};

class BondPositionServiceListener: public ServiceListener<Position<Bond> >
{
private:
  BondRiskService bnd_risk_service;
public:
  BondPositionServiceListener(BondRiskService& bnd_risk): bnd_risk_service(bnd_risk){}
  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(Position<Bond> &data){
    bnd_risk_service.AddPosition(data);
  }

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Position<Bond> &data){}

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(Position<Bond> &data){}

};



template<typename T>
PV01<T>::PV01(const T &_product, double _pv01, long _quantity) :
  product(_product)
{
  pv01 = _pv01;
  quantity = _quantity;
}

template<typename T>
BucketedSector<T>::BucketedSector(const vector<T>& _products, string _name) :
  products(_products)
{
  name = _name;
}

template<typename T>
const vector<T>& BucketedSector<T>::GetProducts() const
{
  return products;
}

template<typename T>
const string& BucketedSector<T>::GetName() const
{
  return name;
}

#endif
