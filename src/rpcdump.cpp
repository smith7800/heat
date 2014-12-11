// Copyright (c) 2009-2012 Bitcoin Developers
// Copyright (c) 2012-2013 The Peercoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

#include "init.h" // for pwalletMain
#include "bitcoinrpc.h"
#include "ui_interface.h"
#include "sha256.h"
#include <boost/lexical_cast.hpp>

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"


#define printf OutputDebugStringF

// using namespace boost::asio;
using namespace json_spirit;
using namespace std;

extern Object JSONRPCError(int code, const string& message);

class CTxDump
{
public:
  CBlockIndex *pindex;
  int64 nValue;
  bool fSpent;
  CWalletTx* ptx;
  int nOut;
  CTxDump(CWalletTx* ptx = NULL, int nOut = -1)
  {
    pindex = NULL;
    nValue = 0;
    fSpent = false;
    this->ptx = ptx;
    this->nOut = nOut;
  }
};

Value importpassphrase(const Array& params, bool fHelp)
{
  if(fHelp || params.size() < 1 || params.size() > 2)
    throw runtime_error(
      "importpassphrase \"<passphrase>\" [label]\n"
      "Adds a private key into your wallet.");
  string strSecret = params[0].get_str();
  uint256 pass = sha256((const u8int*)strSecret.c_str(), strSecret.length());
  CSecret passSecret;
  passSecret.resize(32);
  for (int i = 0; i < 32; i++) {
   passSecret[i] = (pass.begin())[i];
  }
  
  string strLabel = "";
  if(params.size() > 1)
    strLabel = params[1].get_str();

  CBitcoinSecret vchSecret;
  bool fCompressed = true;
  vchSecret.SetSecret( passSecret , fCompressed );
  

  //if(!fGood) throw JSONRPCError(-5, "Invalid private key");
  if(pwalletMain->IsLocked())
    throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
  if(fWalletUnlockMintOnly) // heat: no importprivkey in mint-only mode
    throw JSONRPCError(-102, "Wallet is unlocked for minting only.");

  CKey key;
  CSecret secret = vchSecret.GetSecret(fCompressed);
  key.SetSecret(secret, fCompressed);
  CBitcoinAddress vchAddress = CBitcoinAddress(key.GetPubKey());

  {
    LOCK2(cs_main, pwalletMain->cs_wallet);

    pwalletMain->MarkDirty();
    pwalletMain->SetAddressBookName(vchAddress, strLabel);

    if(!pwalletMain->AddKey(key))
      throw JSONRPCError(-4,"Error adding key to wallet");

    pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true);
    pwalletMain->ReacceptWalletTransactions();
  }

  MainFrameRepaint();

  Object obj;
  obj.push_back(Pair("Secret",        CBitcoinSecret( passSecret, fCompressed ).ToString() ));
  obj.push_back(Pair("Address",       vchAddress.ToString()  ));
  obj.push_back(Pair("Hash",          pass.GetHex()));
  obj.push_back(Pair("Phrase",        strSecret));
  obj.push_back(Pair("Length",        strSecret.length()));
  return obj;
  // return Value::null;
}

Value importprivkey(const Array& params, bool fHelp)
{
  if(fHelp || params.size() < 1 || params.size() > 2)
    throw runtime_error(
      "importprivkey <heatprivkey> [label]\n"
      "Adds a private key (as returned by dumpprivkey) to your wallet.");

  string strSecret = params[0].get_str();
  string strLabel = "";
  if(params.size() > 1)
    strLabel = params[1].get_str();
  CBitcoinSecret vchSecret;
  bool fGood = vchSecret.SetString(strSecret);

  if(!fGood) throw JSONRPCError(-5, "Invalid private key");
  if(pwalletMain->IsLocked())
    throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
  if(fWalletUnlockMintOnly) // heat: no importprivkey in mint-only mode
    throw JSONRPCError(-102, "Wallet is unlocked for minting only.");

  CKey key;
  bool fCompressed;
  CSecret secret = vchSecret.GetSecret(fCompressed);
  key.SetSecret(secret, fCompressed);
  CBitcoinAddress vchAddress = CBitcoinAddress(key.GetPubKey());

  {
    LOCK2(cs_main, pwalletMain->cs_wallet);

    pwalletMain->MarkDirty();
    pwalletMain->SetAddressBookName(vchAddress, strLabel);

    if(!pwalletMain->AddKey(key))
      throw JSONRPCError(-4,"Error adding key to wallet");

    pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true);
    pwalletMain->ReacceptWalletTransactions();
  }

  MainFrameRepaint();

  return Value::null;
}

Value dumpprivkey(const Array& params, bool fHelp)
{
  if(fHelp || params.size() != 1)
    throw runtime_error(
      "dumpprivkey <heataddress>\n"
      "Reveals the private key corresponding to <heataddress>.");

  string strAddress = params[0].get_str();
  CBitcoinAddress address;

  if(!address.SetString(strAddress))
    throw JSONRPCError(-5, "Invalid heat address");
  if(pwalletMain->IsLocked())
    throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
  if(fWalletUnlockMintOnly) // heat: no dumpprivkey in mint-only mode
    throw JSONRPCError(-102, "Wallet is unlocked for minting only.");

  CSecret vchSecret;
  bool fCompressed;
  if(!pwalletMain->GetSecret(address, vchSecret, fCompressed))
    throw JSONRPCError(-4, "Private key for address " + strAddress + " is not known");

  return CBitcoinSecret(vchSecret, fCompressed).ToString();
}
