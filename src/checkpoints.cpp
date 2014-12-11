// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2013 The Peercoin developers
// Copyright (c) 2013-2014 The heat Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "db.h"
#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
  typedef std::map<int, uint256> MapCheckpoints;   // hardened checkpoints

  //
  // What makes a good checkpoint block?
  // + Is surrounded by blocks with reasonable timestamps
  //   (no blocks before with a timestamp after, none after with
  //    timestamp before)
  // + Contains no strange transactions
  //
  static MapCheckpoints mapCheckpoints =
    boost::assign::map_list_of
    ( 0, hashGenesisBlockOfficial )
    ( 9012,   uint256("0x00000006b2c364c71c279977abc8adf528d25263fb3b4fa623a309745d9f6246"))
    ( 9013,   uint256("0x000000064ce948cdba2c2223dd75c3677847a00daded6be78a097db82d616eee"))
    ( 9201,   uint256("0x0000000ab86098c475566cf8f494d131c4d17aa18a26e946f6b6143be4989d43"))
    ( 9401,   uint256("0x0000000538be9059aa111cdee43d7f6c5c4e6581073af1f4478a8705f5817f3b"))
    ( 10198,  uint256("0x000000086631340ce44f7ee72e7125654eef62181a08bacf69b42f797fd7bb4c"))
    ( 10503,  uint256("0x1a433766560d719d5ece18aa190b00fd503d733e2162e511df0998df5c8680f5"))
    ( 15165,  uint256("0x0000017fba5ef709509c7380e3e128a69a5ab3c60b526c8345aff592dc8d8f81"))
    ( 15935,  uint256("0xaf377a2f3be16d3c3d82ad9158a3c24b5e8a7a1af6e315b486a390c651d70ff5"))
    ( 15936,  uint256("0x0000002a4ba8fac73286a3cbcac76610a11f3faebec4ce19c13aca30990684f4"))
    ( 35587,  uint256("0x07450307a456afcbc62d8414913a8b4497f4dc13629337b7db5658aa52877155"))
    ( 43685,  uint256("0xa91550beeed702374a01bd8999669cc9dc6952752f2ede01ef446cb0d2113e6f"))
    ( 121300, uint256("0x6f7f1b5cf35b3be443f747ffdacbc7faba860ec82535dcbe1d1486eed6eea131")) 

    ;

  bool CheckHardened(int nHeight, const uint256 &hash)
  {
    if(fTestNet) // Testnet has no checkpoints
      return true;

    MapCheckpoints::const_iterator i = mapCheckpoints.find(nHeight);
    if(i == mapCheckpoints.end()) 
      return true;
    return hash == i->second;
  }

  bool CheckpointExists(s32int nHeight)
  {
    //the testnet hash no checkpoints
    if(fTestNet)
      return false;

    MapCheckpoints::const_iterator i = mapCheckpoints.find(nHeight);
    return i != mapCheckpoints.end();
  }

  int GetTotalBlocksEstimate()
  {
    if(fTestNet)
      return 0;

    return mapCheckpoints.rbegin()->first;
  }

  CBlockIndex *GetLastCheckpoint(const std::map<uint256, CBlockIndex*> &mapBlockIndex)
  {
    if(fTestNet)
    {
      std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hashGenesisBlock);
      if(t != mapBlockIndex.end())
        return t->second;
      return NULL;
    }

    BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, mapCheckpoints)
    {
      const uint256& hash = i.second;
      std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
      if(t != mapBlockIndex.end())
        return t->second;
    }
    return NULL;
  }

  // heat: synchronized checkpoint (centrally broadcasted)
  uint256 hashSyncCheckpoint = 0;
  uint256 hashPendingCheckpoint = 0;
  CSyncCheckpoint checkpointMessage;
  CSyncCheckpoint checkpointMessagePending;
  uint256 hashInvalidCheckpoint = 0;
  CCriticalSection cs_hashSyncCheckpoint;

  // heat: get last synchronized checkpoint
  CBlockIndex* GetLastSyncCheckpoint()
  {
    LOCK(cs_hashSyncCheckpoint);
    if(!mapBlockIndex.count(hashSyncCheckpoint))
      error("GetSyncCheckpoint: block index missing for current sync-checkpoint %s", 
            hashSyncCheckpoint.ToString().c_str());
    else
      return mapBlockIndex[hashSyncCheckpoint];
    return NULL;
  }

  // heat: only descendant of current sync-checkpoint is allowed
  bool ValidateSyncCheckpoint(uint256 hashCheckpoint)
  {
    if(!mapBlockIndex.count(hashSyncCheckpoint))
      return error("ValidateSyncCheckpoint: block index missing for current sync-checkpoint %s", 
                   hashSyncCheckpoint.ToString().c_str());
    if(!mapBlockIndex.count(hashCheckpoint))
      return error("ValidateSyncCheckpoint: block index missing for received sync-checkpoint %s",
                   hashCheckpoint.ToString().c_str());

    CBlockIndex *pindexSyncCheckpoint = mapBlockIndex[hashSyncCheckpoint];
    CBlockIndex *pindexCheckpointRecv = mapBlockIndex[hashCheckpoint];

    if(pindexCheckpointRecv->nHeight <= pindexSyncCheckpoint->nHeight)
    {
      // Received an older checkpoint, trace back from current checkpoint
      // to the same height of the received checkpoint to verify
      // that current checkpoint should be a descendant block
      CBlockIndex* pindex = pindexSyncCheckpoint;
      while(pindex->nHeight > pindexCheckpointRecv->nHeight)
        if(!(pindex = pindex->pprev))
          return error("ValidateSyncCheckpoint: pprev1 null - block index structure failure");

      if(pindex->GetBlockHash() != hashCheckpoint)
      {
        hashInvalidCheckpoint = hashCheckpoint;
        return error(
          "ValidateSyncCheckpoint: new sync-checkpoint %s is conflicting with current sync-checkpoint %s",
          hashCheckpoint.ToString().c_str(), hashSyncCheckpoint.ToString().c_str());
      }
      return false; // ignore older checkpoint
    }

    // Received checkpoint should be a descendant block of the current
    // checkpoint. Trace back to the same height of current checkpoint
    // to verify.
    CBlockIndex *pindex = pindexCheckpointRecv;
    while(pindex->nHeight > pindexSyncCheckpoint->nHeight)
      if(!(pindex = pindex->pprev))
        return error("ValidateSyncCheckpoint: pprev2 null - block index structure failure");

    if(pindex->GetBlockHash() != hashSyncCheckpoint)
    {
      hashInvalidCheckpoint = hashCheckpoint;
      return error(
        "ValidateSyncCheckpoint: new sync-checkpoint %s is not a descendant of current sync-checkpoint %s",
        hashCheckpoint.ToString().c_str(), hashSyncCheckpoint.ToString().c_str());
    }
    return true;
  }

  bool WriteSyncCheckpoint(const uint256 &hashCheckpoint)
  {
    CTxDB txdb;
    txdb.TxnBegin();
    if(!txdb.WriteSyncCheckpoint(hashCheckpoint))
    {
      txdb.TxnAbort();
      return error("WriteSyncCheckpoint(): failed to write to db sync checkpoint %s",
                   hashCheckpoint.ToString().c_str());
    }
    if(!txdb.TxnCommit())
      return error("WriteSyncCheckpoint(): failed to commit to db sync checkpoint %s",
                   hashCheckpoint.ToString().c_str());
    txdb.Close();

    Checkpoints::hashSyncCheckpoint = hashCheckpoint;
    return true;
  }

  bool AcceptPendingSyncCheckpoint()
  {
    LOCK(cs_hashSyncCheckpoint);
    if(hashPendingCheckpoint != 0 && mapBlockIndex.count(hashPendingCheckpoint))
    {
      if(!ValidateSyncCheckpoint(hashPendingCheckpoint))
      {
        hashPendingCheckpoint = 0;
        checkpointMessagePending.SetNull();
        return false;
      }

      CTxDB txdb;
      CBlockIndex *pindexCheckpoint = mapBlockIndex[hashPendingCheckpoint];
      if(!pindexCheckpoint->IsInMainChain())
      {
        CBlock block;
        if(!block.ReadFromDisk(pindexCheckpoint))
          return error("AcceptPendingSyncCheckpoint: ReadFromDisk failed for sync checkpoint %s",
                       hashPendingCheckpoint.ToString().c_str());
        if(!block.SetBestChain(txdb, pindexCheckpoint))
        {
          hashInvalidCheckpoint = hashPendingCheckpoint;
          return error("AcceptPendingSyncCheckpoint: SetBestChain failed for sync checkpoint %s",
                       hashPendingCheckpoint.ToString().c_str());
        }
      }
      txdb.Close();

      if(!WriteSyncCheckpoint(hashPendingCheckpoint))
        return error("AcceptPendingSyncCheckpoint(): failed to write sync checkpoint %s",
                     hashPendingCheckpoint.ToString().c_str());

      hashPendingCheckpoint = 0;
      checkpointMessage = checkpointMessagePending;
      checkpointMessagePending.SetNull();
      printf("AcceptPendingSyncCheckpoint : sync-checkpoint at %s\n", hashSyncCheckpoint.ToString().c_str());
      // relay the checkpoint
      if(!checkpointMessage.IsNull())
      {
        BOOST_FOREACH(CNode* pnode, vNodes)
          checkpointMessage.RelayTo(pnode);
      }
      return true;
    }
    return false;
  }

  // Automatically select a suitable sync-checkpoint 
  uint256 AutoSelectSyncCheckpoint()
  {
    // Proof-of-work blocks are immediately checkpointed
    // to defend against 51% attack which rejects other miners block 

    // Select the last proof-of-work block
    const CBlockIndex *pindex = GetLastBlockIndex(pindexBest, false);
    // Search forward for a block within max span and maturity window
    while(pindex->pnext && (pindex->GetBlockTime() + CHECKPOINT_MAX_SPAN <= pindexBest->GetBlockTime() || 
                            pindex->nHeight + std::min(6, nCoinbaseMaturity - 20) <= pindexBest->nHeight))
      pindex = pindex->pnext;

    return pindex->GetBlockHash();
  }

  // Check against synchronized checkpoint
  bool CheckSync(const uint256& hashBlock, const CBlockIndex* pindexPrev)
  {
    if(fTestNet) 
      return true; // Testnet has no checkpoints

    int nHeight = pindexPrev->nHeight + 1;

    LOCK(cs_hashSyncCheckpoint);
    // sync-checkpoint should always be accepted block
    assert(mapBlockIndex.count(hashSyncCheckpoint));
    const CBlockIndex *pindexSync = mapBlockIndex[hashSyncCheckpoint];

    if(nHeight > pindexSync->nHeight)
    {
      // trace back to same height as sync-checkpoint
      const CBlockIndex *pindex = pindexPrev;
      while(pindex->nHeight > pindexSync->nHeight)
        if(!(pindex = pindex->pprev))
          return error("CheckSync: pprev null - block index structure failure");

      if(pindex->nHeight < pindexSync->nHeight || pindex->GetBlockHash() != hashSyncCheckpoint)
        return false; // only descendant of sync-checkpoint can pass check
    }

    if(nHeight == pindexSync->nHeight && hashBlock != hashSyncCheckpoint)
      return false; // same height with sync-checkpoint

    if(nHeight < pindexSync->nHeight && !mapBlockIndex.count(hashBlock))
      return false; // lower height than sync-checkpoint

    return true;
  }

  bool WantedByPendingSyncCheckpoint(uint256 hashBlock)
  {
    LOCK(cs_hashSyncCheckpoint);
    if(hashPendingCheckpoint == 0)
      return false;
    if(hashBlock == hashPendingCheckpoint)
      return true;
    if(mapOrphanBlocks.count(hashPendingCheckpoint) 
       && hashBlock == WantedByOrphan(mapOrphanBlocks[hashPendingCheckpoint]))
      return true;
    return false;
  }

  // heat: reset synchronized checkpoint to last hardened checkpoint
  bool ResetSyncCheckpoint()
  {
    LOCK(cs_hashSyncCheckpoint);
    const uint256& hash = mapCheckpoints.rbegin()->second;
    if(mapBlockIndex.count(hash) && !mapBlockIndex[hash]->IsInMainChain())
    {
      // checkpoint block accepted but not yet in main chain
      printf("ResetSyncCheckpoint: SetBestChain to hardened checkpoint %s\n", hash.ToString().c_str());
      CTxDB txdb;
      CBlock block;
      if(!block.ReadFromDisk(mapBlockIndex[hash]))
        return error("ResetSyncCheckpoint: ReadFromDisk failed for hardened checkpoint %s", 
                     hash.ToString().c_str());
      if(!block.SetBestChain(txdb, mapBlockIndex[hash]))
      {
        return error("ResetSyncCheckpoint: SetBestChain failed for hardened checkpoint %s", 
                     hash.ToString().c_str());
      }
      txdb.Close();
    }
    else if(!mapBlockIndex.count(hash))
    {
      // checkpoint block not yet accepted
      hashPendingCheckpoint = hash;
      checkpointMessagePending.SetNull();
      printf("ResetSyncCheckpoint: pending for sync-checkpoint %s\n", hashPendingCheckpoint.ToString().c_str());
    }

    BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, mapCheckpoints)
    {
      const uint256& hash = i.second;
      if(mapBlockIndex.count(hash) && mapBlockIndex[hash]->IsInMainChain())
      {
        if(!WriteSyncCheckpoint(hash))
          return error("ResetSyncCheckpoint: failed to write sync checkpoint %s", hash.ToString().c_str());
        printf("ResetSyncCheckpoint: sync-checkpoint reset to %s\n", hashSyncCheckpoint.ToString().c_str());
        return true;
      }
    }

    return false;
  }

  void AskForPendingSyncCheckpoint(CNode* pfrom)
  {
    LOCK(cs_hashSyncCheckpoint);
    if(pfrom && hashPendingCheckpoint != 0 && (!mapBlockIndex.count(hashPendingCheckpoint)) && 
       (!mapOrphanBlocks.count(hashPendingCheckpoint)))
      pfrom->AskFor(CInv(MSG_BLOCK, hashPendingCheckpoint));
  }

  bool SetCheckpointPrivKey(std::string strPrivKey)
  {
    // Test signing a sync-checkpoint with genesis block
    CSyncCheckpoint checkpoint;
    checkpoint.hashCheckpoint = hashGenesisBlock;
    CDataStream sMsg(SER_NETWORK, PROTOCOL_VERSION);
    sMsg << (CUnsignedSyncCheckpoint)checkpoint;
    checkpoint.vchMsg = std::vector<unsigned char>(sMsg.begin(), sMsg.end());

    std::vector<unsigned char> vchPrivKey = ParseHex(strPrivKey);
    CKey key;
    key.SetPrivKey(CPrivKey(vchPrivKey.begin(), vchPrivKey.end())); // if key is not correct openssl may crash
    if(!key.Sign(Hash(checkpoint.vchMsg.begin(), checkpoint.vchMsg.end()), checkpoint.vchSig))
      return false;

    // Test signing successful, proceed
    CSyncCheckpoint::strMasterPrivKey = strPrivKey;

    return true;
  }

  bool SendSyncCheckpoint(uint256 hashCheckpoint)
  {
    CSyncCheckpoint checkpoint;
    checkpoint.hashCheckpoint = hashCheckpoint;
    CDataStream sMsg(SER_NETWORK, PROTOCOL_VERSION);
    sMsg << (CUnsignedSyncCheckpoint)checkpoint;
    checkpoint.vchMsg = std::vector<unsigned char>(sMsg.begin(), sMsg.end());

    if(CSyncCheckpoint::strMasterPrivKey.empty())
      return error("SendSyncCheckpoint: Checkpoint master key unavailable.");

    std::vector<unsigned char> vchPrivKey = ParseHex(CSyncCheckpoint::strMasterPrivKey);
    CKey key;
    key.SetPrivKey(CPrivKey(vchPrivKey.begin(), vchPrivKey.end())); // if key is not correct openssl may crash
    if(!key.Sign(Hash(checkpoint.vchMsg.begin(), checkpoint.vchMsg.end()), checkpoint.vchSig))
      return error("SendSyncCheckpoint: Unable to sign checkpoint, check private key?");

    if(!checkpoint.ProcessSyncCheckpoint(NULL))
    {
      printf("WARNING: SendSyncCheckpoint: Failed to process checkpoint.\n");
      return false;
    }

    // Relay checkpoint
    {
      LOCK(cs_vNodes);
      BOOST_FOREACH(CNode* pnode, vNodes)
        checkpoint.RelayTo(pnode);
    }
    return true;
  }

  // Is the sync-checkpoint outside maturity window?
  bool IsMatureSyncCheckpoint()
  {
    LOCK(cs_hashSyncCheckpoint);
    // sync-checkpoint should always be accepted block
    assert(mapBlockIndex.count(hashSyncCheckpoint));
    const CBlockIndex* pindexSync = mapBlockIndex[hashSyncCheckpoint];
    return (nBestHeight >= pindexSync->nHeight + nCoinbaseMaturity ||
            pindexSync->GetBlockTime() + nStakeMinAge < GetAdjustedTime());
  }

  // Is the sync-checkpoint too old?
  bool IsSyncCheckpointTooOld(unsigned int nSeconds)
  {
    LOCK(cs_hashSyncCheckpoint);
    // sync-checkpoint should always be accepted block
    assert(mapBlockIndex.count(hashSyncCheckpoint));
    const CBlockIndex *pindexSync = mapBlockIndex[hashSyncCheckpoint];
    return false;
    return (pindexSync->GetBlockTime() + nSeconds < GetAdjustedTime());
  }
}

// heat: sync-checkpoint master key
const std::string CSyncCheckpoint::strMasterPubKey = "027f90fb303efc0109b09ebe10cbea86919a1494629b4f997ccb61597d154768d0";

std::string CSyncCheckpoint::strMasterPrivKey = "";

// heat: verify signature of sync-checkpoint message
bool CSyncCheckpoint::CheckSignature()
{
  CKey key;
  if(!key.SetPubKey(ParseHex(CSyncCheckpoint::strMasterPubKey)))
    return error("CSyncCheckpoint::CheckSignature() : SetPubKey failed");
  if(!key.Verify(Hash(vchMsg.begin(), vchMsg.end()), vchSig))
    return error("CSyncCheckpoint::CheckSignature() : verify signature failed");

  // Now unserialize the data
  CDataStream sMsg(vchMsg, SER_NETWORK, PROTOCOL_VERSION);
  sMsg >> *(CUnsignedSyncCheckpoint*)this;
  return true;
}

// heat: process synchronized checkpoint
bool CSyncCheckpoint::ProcessSyncCheckpoint(CNode *pfrom)
{
  if(!CheckSignature())
    return error("ProcessSyncCheckpoint() : Check Signature failed");

  LOCK(Checkpoints::cs_hashSyncCheckpoint);
  if(!mapBlockIndex.count(hashCheckpoint))
  {
    // We haven't received the checkpoint chain, keep the checkpoint as pending
    Checkpoints::hashPendingCheckpoint = hashCheckpoint;
    Checkpoints::checkpointMessagePending = *this;
    printf("ProcessSyncCheckpoint() : pending for sync-checkpoint %s\n", hashCheckpoint.ToString().c_str());
    // Ask this guy to fill in what we're missing
    if(pfrom)
    {
      pfrom->PushGetBlocks(pindexBest, hashCheckpoint);
      // ask directly as well in case rejected earlier by duplicate
      // proof-of-stake because getblocks may not get it this time
      pfrom->AskFor(CInv(MSG_BLOCK, 
                         mapOrphanBlocks.count(hashCheckpoint) ?
                         WantedByOrphan(mapOrphanBlocks[hashCheckpoint]) : hashCheckpoint));
    }
    return false;
  }

  if(!Checkpoints::ValidateSyncCheckpoint(hashCheckpoint))
    return false;

  CTxDB txdb;
  CBlockIndex* pindexCheckpoint = mapBlockIndex[hashCheckpoint];
  if(!pindexCheckpoint->IsInMainChain())
  {
    // checkpoint chain received but not yet main chain
    CBlock block;
    if(!block.ReadFromDisk(pindexCheckpoint))
      return error("ProcessSyncCheckpoint() : ReadFromDisk failed for sync checkpoint %s",
                   hashCheckpoint.ToString().c_str());
    if(!block.SetBestChain(txdb, pindexCheckpoint))
    {
      Checkpoints::hashInvalidCheckpoint = hashCheckpoint;
      return error("ProcessSyncCheckpoint() : SetBestChain failed for sync checkpoint %s", 
                   hashCheckpoint.ToString().c_str());
    }
  }
  txdb.Close();

  if(!Checkpoints::WriteSyncCheckpoint(hashCheckpoint))
    return error("ProcessSyncCheckpoint() : failed to write sync checkpoint %s", hashCheckpoint.ToString().c_str());

  Checkpoints::checkpointMessage = *this;
  Checkpoints::hashPendingCheckpoint = 0;
  Checkpoints::checkpointMessagePending.SetNull();
  printf("ProcessSyncCheckpoint() : sync-checkpoint at %s\n", hashCheckpoint.ToString().c_str());
  return true;
}
