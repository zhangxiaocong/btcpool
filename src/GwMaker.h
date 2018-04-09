/*
 The MIT License (MIT)

 Copyright (C) 2017 RSK Labs Ltd.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

/**
  File: GwMaker.h
  Purpose: Poll RSK node to get new work and send it to Kafka "RawGw" topic

  @author Martin Medina
  @copyright RSK Labs Ltd.
  @version 1.0 30/03/17 

  maintained by HaoLi (fatrat1117) and YihaoPeng since Feb 20, 2018
*/

#ifndef GW_MAKER_H_
#define GW_MAKER_H_

#include "Common.h"
#include "Kafka.h"
#include "utilities_js.hpp"


struct GwDefinition
{
  string chainType_;
  bool enabled_;

  string rpcAddr_;
  string rpcUserPwd_;
  uint32 rpcInterval_;

  string rawGwTopic_;
};

class GwHandler {
  public:
    virtual ~GwHandler() = 0; // mark it's an abstract class
    virtual void init(const GwDefinition &def) { def_ = def; }
    
    // read-only definition
    virtual const GwDefinition& def() { return def_; }

    // Interface with the GwMaker.
    // There is a default implementation that use virtual functions below.
    // If the implementation does not meet the requirements, you can overload it
    // and ignore all the following virtual functions.
    virtual string makeRawGwMsg();

  protected:

    // These virtual functions make it easier to implement the makeRawGwMsg() interface.
    // In most cases, you just need to override getRequestData() and processRawGw().
    // If you have overloaded makeRawGwMsg() above, you can ignore all the following functions.

    // Receive rpc response and generate RawGw message for the pool.
    virtual string processRawGw(const string &gw) { return ""; }

    // Call RPC `getwork` and get the response.
    virtual bool callRpcGw(string &resp);

    // Body of HTTP POST used by callRpcGw().
    // return "" if use HTTP GET.
    virtual string getRequestData() { return ""; }
    // HTTP header `User-Agent` used by callRpcGw().
    virtual string getUserAgent() { return "curl"; }

    // blockchain and RPC-server definitions
    GwDefinition def_;
};

class GwHandlerRsk : public GwHandler 
{
  bool checkFields(JsonNode &r);
  string constructRawMsg(JsonNode &r);
  string processRawGw(const string &gw);

  string getRequestData() { return "{\"jsonrpc\": \"2.0\", \"method\": \"mnr_getWork\", \"params\": [], \"id\": 1}"; }
};

class GwHandlerEth : public GwHandler
{
  bool checkFields(JsonNode &r);
  string constructRawMsg(JsonNode &r);
  string processRawGw(const string &gw);

  string getRequestData() { return "{\"jsonrpc\": \"2.0\", \"method\": \"eth_getWork\", \"params\": [], \"id\": 1}"; }
};

class GwHandlerSia : public GwHandler 
{
  string processRawGw(const string &gw);

  string getRequestData() { return ""; }
  string getUserAgent() { return "Sia-Agent"; }
};


class GwMaker {
  shared_ptr<GwHandler> handle_;
  atomic<bool> running_;

private:
  uint32_t kRpcCallInterval_;

  string kafkaBrokers_;
  KafkaProducer kafkaProducer_;

  string makeRawGwMsg();
  void submitRawGwMsg();
  void kafkaProduceMsg(const void *payload, size_t len);

public:
  GwMaker(shared_ptr<GwHandler> handle, const string &kafkaBrokers);
  virtual ~GwMaker();

  bool init();
  void stop();
  void run();

  // for logs
  string getChainType() { return handle_->def().chainType_; }
  string getRawGwTopic() { return handle_->def().rawGwTopic_; }
};

#endif
