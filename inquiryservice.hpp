/**
 * inquiryservice.hpp
 * Defines the data types and Service for customer inquiries.
 *
 * @author Breman Thuraisingham
 */
#ifndef INQUIRY_SERVICE_HPP
#define INQUIRY_SERVICE_HPP

#include "soa.hpp"
#include "tradebookingservice.hpp"

// Various inqyury states
enum InquiryState { RECEIVED, QUOTED, DONE, REJECTED, CUSTOMER_REJECTED };

/**
 * Inquiry object modeling a customer inquiry from a client.
 * Type T is the product type.
 */
template<typename T>
class Inquiry
{

public:

  // ctor for an inquiry
  Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state);

  // Get the inquiry ID
  const string& GetInquiryId() const;

  // Get the product
  const T& GetProduct() const;

  // Get the side on the inquiry
  Side GetSide() const;

  // Get the quantity that the client is inquiring for
  long GetQuantity() const;

  // Get the price that we have responded back with
  double GetPrice() const;

  //set the price
  void SetPrice(double p){price=p;}

  // Get the current state on the inquiry
  InquiryState GetState() const;
  //set state
  void SetState(InquiryState s){state=s;}

private:
  string inquiryId;
  T product;
  Side side;
  long quantity;
  double price;
  InquiryState state;

};

/**
 * Service for customer inquirry objects.
 * Keyed on inquiry identifier (NOTE: this is NOT a product identifier since each inquiry must be unique).
 * Type T is the product type.
 */
template<typename T>
class InquiryService : public Service<string,Inquiry <T> >
{

public:

  // Send a quote back to the client
  virtual void SendQuote(const string &inquiryId, double price) = 0;

  // Reject an inquiry from the client
  virtual void RejectInquiry(const string &inquiryId) = 0;

};
class BondInquiryService;
//publish only connector
class BondPublishIqConnector: public Connector<Inquiry<Bond> >
{
public:
    virtual void Publish(Inquiry<Bond> &data);
    virtual void SetPublish(Inquiry<Bond> &data, BondInquiryService& b_iqure);
};
class BondInquiryService: public InquiryService<Bond>
{
private:
  map<string, Inquiry<Bond> > bondInquiryCache;
  vector< ServiceListener<Inquiry<Bond> >* > bondInquiryListeners;
  BondPublishIqConnector b_publish;
public:
  BondInquiryService(BondPublishIqConnector& src):b_publish(src){}
  // Get data on our service given a key
  virtual Inquiry<Bond>& GetData(string key){return bondInquiryCache.find(key)->second;}

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Inquiry<Bond> &data);

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Inquiry<Bond> > *listener){bondInquiryListeners.push_back(listener);}

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<Inquiry<Bond> >* >& GetListeners() const {return bondInquiryListeners;}
  //send a quote back to client
  virtual void SendQuote(const string& inquiryId, double price);
  //reject an inquiry if a sell inquiry has price higher than 100 or a buy inquiry has price lower than 100
  virtual void RejectInquiry(const string& inquiryId){} 

};

//subscribe only connector
class BondInquiryConnector: public Connector<Inquiry<Bond> >
{
private:
   int counter;
public:
   BondInquiryConnector(){counter=0;}//constructor
   // Publish data to the Connector
  virtual void Publish(Inquiry<Bond> &data){}//do nothing
  //subscribe and return subscribed data
  virtual void Subscribe(BondInquiryService& b_inquire, map<string, Bond> m_bond);
};

class BondInquiryListener: public ServiceListener<Inquiry<Bond> >
{
private:
  BondInquiryService& b_inquire;
public:
  BondInquiryListener(BondInquiryService& src):b_inquire(src){}
   // Listener callback to process an add event to the Service
  virtual void ProcessAdd(Inquiry<Bond> &data);

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Inquiry<Bond> &data){}

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(Inquiry<Bond> &data){data.SetState(DONE);}
};

template<typename T>
Inquiry<T>::Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state) :
  product(_product)
{
  inquiryId = _inquiryId;
  side = _side;
  quantity = _quantity;
  price = _price;
  state = _state;
}

template<typename T>
const string& Inquiry<T>::GetInquiryId() const
{
  return inquiryId;
}

template<typename T>
const T& Inquiry<T>::GetProduct() const
{
  return product;
}

template<typename T>
Side Inquiry<T>::GetSide() const
{
  return side;
}

template<typename T>
long Inquiry<T>::GetQuantity() const
{
  return quantity;
}

template<typename T>
double Inquiry<T>::GetPrice() const
{
  return price;
}

template<typename T>
InquiryState Inquiry<T>::GetState() const
{
  return state;
}

//flow into service
void BondInquiryConnector::Subscribe(BondInquiryService& b_inquire, map<string, Bond> m_bond){
    ifstream file;
    file.open("./Input/inquiries.txt");//open file
    string line;//store one line
    getline(file,line);//read header line
    try{
      for(int i=0;i<=counter;++i)
        getline(file,line);//read line
      if(line.length()<4){
        throw "end of file\n";
        //a normal data entry line length can never be less than 4
      }
    }
    catch(...){
      std::cout<<"reached end of file\n"; 
      return;
    }
    ++counter;//update counter
    vector<string> datalines;//store info about readed line
    vector<string> priceparts;//store parts of prices
    boost::split(datalines,line,boost::is_any_of(","));//split line
    string inquireId=datalines[0];//get inquiry id
    string bondId=datalines[1];//get bond id string
    string strSide=datalines[2];//get Side string
    Side theside;
    if(strSide=="SELL"){
      theside=SELL;
    }
    else{
      theside=BUY;
    }
    string strQty=datalines[3];//get quantity
    long qty=stol(strQty,nullptr);//convert to long
    string strP=datalines[4];//get price
    boost::split(priceparts,strP,boost::is_any_of("-"));//split price
    string p1=priceparts[0];//get part 1 of price
    string p2=priceparts[1];//get part 2 of price
    double bid1num=0;//get price num
    for(int i=0;i<p1.length();++i){
      int current=int(p1[i]-'0');//get current digit
      bid1num+=current*pow(10,p1.length()-i-1);//get first part of price
    }
    double bid2num=10.0*int(p2[0]-'0')+int(p2[1]-'0');//get bid2num
    bid2num=bid2num/32.0;
    double bid3num=int(p2[2]-'0')/256.0;//get part 3 of price
    double Ptotal=bid1num+bid2num+bid3num;//sum up price
    Bond bnd=m_bond[bondId];//get bond
    Inquiry<Bond> iq_bnd(inquireId,bnd,theside,qty,Ptotal,RECEIVED);
    b_inquire.OnMessage(iq_bnd);//flow data to service
}

void BondInquiryService::OnMessage(Inquiry<Bond> &data){
  //get inquiry state
  InquiryState state1=data.GetState();
  string iqId=data.GetInquiryId();//get inquiry id
  map<string, Inquiry<Bond> >::iterator it=bondInquiryCache.find(iqId);
  if(state1==RECEIVED){
     //for the case of received
    if(it==bondInquiryCache.end()){
      bondInquiryCache.insert(make_pair(iqId,data));//insert data
    }
    else{
      bondInquiryCache.erase(iqId);//erase old record
      bondInquiryCache.insert(make_pair(iqId,data));//insert data
    }
    for(int i=0;i<bondInquiryListeners.size();++i){
      //processAdd is called for receive state process
      bondInquiryListeners[i]->ProcessAdd(data);
    }
  }
  else if(state1==QUOTED){
    //update it immediately
    for(int i=0;i<bondInquiryListeners.size();++i){
      //processupdate is called
      bondInquiryListeners[i]->ProcessUpdate(data);
    }
    if(it==bondInquiryCache.end()){
      bondInquiryCache.insert(make_pair(iqId,data));//insert data
    }
    else{
      bondInquiryCache.erase(iqId);//erase old record
      bondInquiryCache.insert(make_pair(iqId,data));//insert data
    }
  }
}

void BondInquiryListener::ProcessAdd(Inquiry<Bond>& data){
   string iqId=data.GetInquiryId();//get inquiry id of data
   double p=100;//set quote price to be 100
   b_inquire.SendQuote(iqId,p);//send a quote of 100
}

void BondInquiryService::SendQuote(const string& inquiryId, double price){
  map<string, Inquiry<Bond> >::iterator it=bondInquiryCache.find(inquiryId);
  if(it==bondInquiryCache.end()){
    //such inquiry does not exist
    cout<<"Cache miss\n";
    return;
  }
  it->second.SetPrice(price);//reset the price
  b_publish.SetPublish(it->second,*this);//publish it
}

 void BondPublishIqConnector::Publish(Inquiry<Bond> &data){
    //transit to Quoted state
    data.SetState(QUOTED);
  }
  void BondPublishIqConnector::SetPublish(Inquiry<Bond> &data, BondInquiryService& b_iqure){
     Publish(data);
     b_iqure.OnMessage(data);//flow into service
  }

#endif
