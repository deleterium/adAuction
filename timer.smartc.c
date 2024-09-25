#program name AdAuctionTimer
#program description 1) Receive a transaction from the adAuction contract;\
 2) Sleeps one week; 3) Send all balance to adAuction contract to trigger\
 a new auction.
#program activationAmount 5000_0000

#program contract 444
#pragma verboseAssembly
#pragma optimizationLevel 3
// #pragma version 2.1.1
#pragma maxConstVars 1

// SIMULATOR | TESTNET | MAINNET
#define SIMULATOR

#ifdef SIMULATOR
  #define TIME_INTERVAL 8
#else
  #ifdef TESTNET
    #define TIME_INTERVAL 358
  #else
    // MAINNET
    #define TIME_INTERVAL 2518
    #program codeHashId 3960370383754817351
  #endif
#endif

/* End of configurations */

/* start of global variables */
struct TXINFO {
    long txid;
    long sender;
} currentTX;

long adAuctionId;
long sleepBlocks;
long goingToSleep;
/* End of global variables */

// Initial setup
const sleepBlocks = TIME_INTERVAL;
// end of initial setup

void main () {
    do {
        goingToSleep = false;
        while ((currentTX.txid = getNextTx()) != 0) {
            // get details
            currentTX.sender = getSender(currentTX.txid);
            if (adAuctionId == 0) {
                adAuctionId = currentTX.sender;
            }
            if (currentTX.sender != adAuctionId) {
                continue;
            }
            goingToSleep = true;
        }
        if (goingToSleep) {
            sleep sleepBlocks;
            sendBalance(adAuctionId);
        }
    } while (goingToSleep);
}

/* Testcase 1: regular use
[
  {
    // start. Expect timer start
    "blockheight": 2,    "sender": "555",    "recipient": "444",    "amount": "5000_0000n"
  },
  {
    // Simulated incoming message from unauthorized address. Expect nothing.
    "blockheight": 4,    "sender": "7987987",    "recipient": "444",    "amount": "1_2000_0000n"
  },
  // Expect sending message at block 11
  {
    // Simulated incoming message from unauthorized address. Expect not trigger timer.
    "blockheight": 13,    "sender": "766767",    "recipient": "444",    "amount": "5000_0000n"
  },
  {
    // Simulated incoming message from registered and entering sleep. Sending at block 27
    "blockheight": 18,    "sender": "555",    "recipient": "444",    "amount": "5000_0000n"
  }
]
*/
