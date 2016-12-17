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
#include <tuple>

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
  BucketedSector():name("sector"){}//constructor
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
  virtual const PV01<BucketedSector<Bond> > GetBucketedRisk(const BucketedSector<T> &sector) const=0;

};
using SectorsRisk=tuple<PV01<BucketedSector<Bond> >, PV01<BucketedSector<Bond> >, PV01<BucketedSector<Bond> > >;
//implement risk service for bond
class BondRiskService: public RiskService<Bond>
{
private:
  map<string, PV01<Bond> > bondRiskCache; //keep a local record for pv01
  vector<ServiceListener<PV01<Bond> >* > bondRiskListeners;
  vector<ServiceListener<SectorsRisk>* > bondSectorRiskListeners;
  map<string, double> bondPV01;//map each bond to one pv01 value
public:
  //bondPV01_ must contain all 6 bonds pv01 info
  BondRiskService(map<string,double>& bondPV01_, map<string,Bond> m_bond):bondPV01(bondPV01_){
    for(map<string, Bond>::iterator it=m_bond.begin();it!=m_bond.end();++it){
      Bond bnd=it->second;//get second
      string bondid=it->first;//get first
      double pv=bondPV01.find(bondid)->second;//get pv
      PV01<Bond> thepv01(bnd,pv,0);//construct one
      bondRiskCache.insert(make_pair(bondid,thepv01));
    }
  }
  void UpdateBondPV01(string bondid, double newpv01);

   // Get data on our service given a key
  virtual PV01<Bond>& GetData(string key){return bondRiskCache.find(key)->second;}

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(PV01<Bond> &data){}//do nothing as no need for connector

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<PV01<Bond> > *listener){bondRiskListeners.push_back(listener);}
  virtual void AddListener(ServiceListener<SectorsRisk>* listener){
    bondSectorRiskListeners.push_back(listener);
  }

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<PV01<Bond> >* >& GetListeners() const{return bondRiskListeners;}
  // Add a position that the service will risk
  virtual void AddPosition(Position<Bond> &position);

  // Get the bucketed risk for the bucket sector
  virtual const PV01<BucketedSector<Bond> > GetBucketedRisk(const BucketedSector<Bond> &sector) const;
};

class BondPositionServiceListener: public ServiceListener<Position<Bond> >
{
private:
  BondRiskService& bnd_risk_service;
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

void BondRiskService::UpdateBondPV01(string bondid, double newpv01){
    //assume bondPV01 always contain all bonds'pv01 info
    bondPV01[bondid]=newpv01;
    //get the corresponding record in cache
    map<string,PV01<Bond> >::iterator the_pv01=bondRiskCache.find(bondid);
    if(the_pv01!=bondRiskCache.end()){
      //only when the corresponding cache exists for the bond, it is necessary to update
      the_pv01->second.SetPV01(newpv01);
      for(int i=0;i<bondRiskListeners.size();++i){
        //update
        bondRiskListeners[i]->ProcessAdd(the_pv01->second);//use update when value of pv01 changes
      }
      
       //three vectors for three sectors
      vector<Bond> v_front, v_belly, v_longend;
      //iterate the cache
      for(map<string, PV01<Bond> >::iterator it1=bondRiskCache.begin();it1!=bondRiskCache.end();++it1){
      Bond currentBnd=it1->second.GetProduct();
      if(currentBnd.GetSectorType()==FrontEnd){
        //push to v_front
        v_front.push_back(currentBnd);
      }
      else if(currentBnd.GetSectorType()==Belly){
        v_belly.push_back(currentBnd);//push to v_belly
      }
      else{
        v_longend.push_back(currentBnd);//push to longend
      }
     }
     BucketedSector<Bond> b_fend(v_front,"FrontEnd"), b_belly(v_belly,"Belly"),b_longend(v_longend,"LongEnd");
      //get updated sector pv01
     PV01<BucketedSector<Bond> > pv01_fend=GetBucketedRisk(b_fend), pv01_belly=GetBucketedRisk(b_belly);
     PV01<BucketedSector<Bond> > pv01_longend=GetBucketedRisk(b_longend);
     SectorsRisk s_risks(pv01_fend,pv01_belly,pv01_longend);//construct sectors risk
    //update through listeners
     for(int i=0;i<bondSectorRiskListeners.size();++i){
       bondSectorRiskListeners[i]->ProcessUpdate(s_risks);
     }

    }

  }

const PV01<BucketedSector<Bond> > BondRiskService::GetBucketedRisk(const BucketedSector<Bond> &sector) const{
    //assume all bonds in sector has pv01 record in cache
    vector<Bond> bonds=sector.GetProducts();
    double risk_bucket=0;
    long sum_quantity=0;
    for(int i=0;i<bonds.size();++i){
      //iterate bonds
      Bond bnd=bonds[i];//get bond
      string bid=bnd.GetProductId();//get bond id
      long q;
      PV01<Bond>  thepv01(bondRiskCache.find(bid)->second);//get the pv01 of this bond
      q=thepv01.GetQuantity();//get quantity of the associated pv01
      q=abs(q);//always set q to be positive in calculation of pv01
      risk_bucket+=double(q)*thepv01.GetPV01();//get accumulate risk of the bucket
      sum_quantity+=q;//get the sum of associated products
    }
    double bucket_pv01;
    if(sum_quantity>0){
      bucket_pv01=risk_bucket/double(sum_quantity);//compute pv01 for bucket
    }
    else{
      bucket_pv01=0;
    }
    PV01<BucketedSector<Bond> > sector_pv01(sector, bucket_pv01, sum_quantity);//get pv01 of bucket
    return sector_pv01;
  }

void BondRiskService::AddPosition(Position<Bond> &position){
    Bond bnd=position.GetProduct();//get bond of the position
    //three vectors for three sectors
    vector<Bond> v_front, v_belly, v_longend;
    //iterate the cache
    for(map<string, PV01<Bond> >::iterator it1=bondRiskCache.begin();it1!=bondRiskCache.end();++it1){
      Bond currentBnd=it1->second.GetProduct();
      if(currentBnd.GetSectorType()==FrontEnd){
        //push to v_front
        v_front.push_back(currentBnd);
      }
      else if(currentBnd.GetSectorType()==Belly){
        v_belly.push_back(currentBnd);//push to v_belly
      }
      else{
        v_longend.push_back(currentBnd);//push to longend
      }
    }
    BucketedSector<Bond> b_fend(v_front,"FrontEnd"), b_belly(v_belly,"Belly"),b_longend(v_longend,"LongEnd");
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
        bondRiskListeners[i]->ProcessAdd(the_pv01->second);//invoke listeners for add
      }
    }
    //get updated sector pv01
    PV01<BucketedSector<Bond> > pv01_fend=GetBucketedRisk(b_fend), pv01_belly=GetBucketedRisk(b_belly);
    PV01<BucketedSector<Bond> > pv01_longend=GetBucketedRisk(b_longend);
    SectorsRisk s_risks(pv01_fend,pv01_belly,pv01_longend);//construct sectors risk
    //update through listeners
    for(int i=0;i<bondSectorRiskListeners.size();++i){
      bondSectorRiskListeners[i]->ProcessUpdate(s_risks);
    }
  }



#endif
