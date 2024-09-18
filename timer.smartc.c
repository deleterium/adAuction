#program name AdAuctionTimer
#program description 1) Receive a transaction from the Lotto contract;\
 2) Sleeps one week; 3) Send all balance to Lotto contract to trigger\
 a new draw.
#program activationAmount 5000_0000

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
long phase;
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
            if (phase == 0) {
                readShortMessage(currentTX.txid, &adAuctionId, 1);
                if (adAuctionId == 0) {
                    continue;
                }
                phase = 1;
                continue;
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

/* testcase 1: wrong messsages, no sleep
[
  {
    // wrong message
    "blockheight": 4,    "sender": "555",    "recipient": "1000n",    "amount": "5000_0000n"
  },
  {
    // right message
    "blockheight": 6,    "sender": "555",    "recipient": "1000n",    "amount": "5000_0000n", "messageHex": "e703000000000000"
  },
  // Expecting to set auction contract id to 999 and no sleep/draw activation.
]

*/

/* Testcase 2: regular use
[
  {
    // right message
    "blockheight": 2,    "sender": "555",    "recipient": "1000n",    "amount": "5000_0000n", "messageHex": "e703000000000000"
  },
  {
    // Simulated incoming message from unauthorized address
    "blockheight": 4,    "sender": "7987987",    "recipient": "1000n",    "amount": "1_2000_0000n"
  },
  // Expecting timer not active
  {
    // Simulated incoming message from auction contract
    "blockheight": 6,    "sender": "999",    "recipient": "1000n",    "amount": "1_2000_0000n"
  },
  // Expecting to sleep and send draw activation on round 15.
  {
    // Simulated incoming message from lotto contract
    "blockheight": 16,    "sender": "999",    "recipient": "1000n",    "amount": "1_2000_0000n"
  }
  // Expecting to sleep and send draw activation on round 25.
]
*/
