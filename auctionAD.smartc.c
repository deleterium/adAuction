#program name AdAuction
#program description Contract to auction an advertisement at block explorer. It also stores the information about winners and ad details.
#define ACTIVATION_AMOUNT 1_0000_0000
#program activationAmount ACTIVATION_AMOUNT

#pragma verboseAssembly
// #pragma optimizationLevel 3
// #pragma version 2.1.1
#pragma maxAuxVars 4
#pragma maxConstVars 4

/* configuration */
#define TIMER_CONTRACT 444
#define CONTRACT_CREATOR_PERCENTAGE 2
#define CONTRACT_CREATOR_ACCOUNT 1001
#define MINIMUM_BID 1_0000_0000
#define MINIMUM_BID_STEP 1_0000_0000
/* end of configuration */

#define KEEP_BALANCE 10_0000_0000
#define ACTIVATE_TIMER() (sendAmount(ACTIVATION_AMOUNT + 4000_0000, TIMER_CONTRACT))

/* start of global variables */
struct TXINFO {
    long txId;
    long sender;
    long amount;
    long command;
} currentTX;
struct MESSAGE {
    long notAuthorized[4];
    long missingCommand[4];
    long OK[4];
    long setYourAd[4];
    long bidTooLow[4];
    long bidSuperseeded[4];
    long bidAccepted[4];
    long adOnline[4];
    long topUpContract[4];
    long adSet[4];
} message;
struct AUCTION {
    long minimum;
    long step;
    long bestBid;
    long bestBidUser;
} auction;
long onwer;
/* end of global variables */

void firstRun() {
    onwer = getCreator();
    const message.notAuthorized[] = 'Not authorized                  ';
    const message.missingCommand[] = 'Please specify a command...     ';
    const message.OK[] = 'Command suceed!                 ';
    const message.setYourAd[] = 'Set your Ad details before bid. ';
    const message.bidTooLow[] = 'Refused. Your bid was too low.  ';
    const message.bidSuperseeded[] = 'Your bid was superseeded.       ';
    const message.bidAccepted[] = 'Your bid was accepted!          ';
    const message.adOnline[] = 'Your Ad is online!              ';
    const message.topUpContract[] = 'Funds added!                    ';
    const message.adSet[] = 'Your Ad has been set!           ';
    ACTIVATE_TIMER();
    setMapValue(0, 2, 1); // start suspended state
    auction.minimum=MINIMUM_BID;  // start minimum bid amount
    setMapValue(1, 0, auction.minimum);
    auction.step=MINIMUM_BID_STEP;  // start minimum bid step increase amount
    setMapValue(1, 1, auction.step);
    auction.bestBid=auction.bestBidUser=0;
}

void main(void) {
    long dispatchAuctionEnd = false;
    while ((currentTX.txId = getNextTx()) != 0) {
        currentTX.sender = getSender(currentTX.txId);
        currentTX.amount = getAmount(currentTX.txId);
        readShortMessage(currentTX.txId, &currentTX.command, 1);
        if (currentTX.sender == onwer) {
            processOwnerCommands();
            continue;
        }
        switch (currentTX.command) {
        case 'bid':
            processBid();
            break;
        case "":
            if (currentTX.sender == TIMER_CONTRACT) {
                dispatchAuctionEnd = true;
                break;
            }
            sendAmountAndMessage(currentTX.amount, message.missingCommand, currentTX.sender);
            break;
        default:
            setMapValue(3, currentTX.sender, currentTX.txId);
            sendAmountAndMessage(currentTX.amount, message.adSet, currentTX.sender);
        }
    }
    if (dispatchAuctionEnd) {
        auctionEnd();
    }
}

void processOwnerCommands() {
    switch (currentTX.command) {
    case 'bid':
        processBid();
        return;
    case 'suspend':
        setMapValue(0, 2, 1);
        break;
    case 'resume':
        setMapValue(0, 2, 0);
        break;
    case 'minimum':
        auction.minimum = currentTX.amount;
        setMapValue(1, 0, auction.minimum);
        break;
    case 'step':
        auction.step = currentTX.amount;
        setMapValue(1, 1, auction.step);
        break;
    case "":
        sendMessage(message.topUpContract, currentTX.sender);
        // Do not return the signa. Used to top up balance.
        return;
    default:
        // setting default ad transactionID
        setMapValue(0, 0, currentTX.txId);
        setMapValue(3, currentTX.sender, currentTX.txId);
    }
    sendAmountAndMessage(currentTX.amount, message.OK, currentTX.sender);
}

void processBid() {
    long bidderAd = getMapValue(3, currentTX.sender);
    if (bidderAd == 0) {
        sendAmountAndMessage(currentTX.amount, message.setYourAd, currentTX.sender);
        return;
    }
    if ((auction.bestBid == 0 && currentTX.amount < auction.minimum) ||
        (auction.bestBid != 0 && currentTX.amount < auction.bestBid + auction.step)) {
        sendAmountAndMessage(currentTX.amount, message.bidTooLow, currentTX.sender);
        return;
    }
    if (auction.bestBid != 0) {
        // There was a previous bid. Refund it.
        sendAmountAndMessage(auction.bestBid, message.bidSuperseeded, auction.bestBidUser);
    }
    auction.bestBid = currentTX.amount;
    auction.bestBidUser = currentTX.sender;
    sendMessage(message.bidAccepted, currentTX.sender);
    setMapValue(1, 2, currentTX.sender);
    setMapValue(1, 3, currentTX.amount);
    setMapValue(1, 4, bidderAd);
}

void auctionEnd() {
    ACTIVATE_TIMER();
    if (auction.bestBid == 0) {
        setMapValue(0, 2, 1);
        return;
    }
    setMapValue(0, 1, getMapValue(1, 4));
    setMapValue(0, 2, 0);
    sendMessage(message.adOnline, auction.bestBidUser);
    long incomeBalance = getCurrentBalance() - KEEP_BALANCE;
    sendAmount((100-CONTRACT_CREATOR_PERCENTAGE)*incomeBalance/100, onwer);
    sendAmount(CONTRACT_CREATOR_PERCENTAGE*incomeBalance/100, CONTRACT_CREATOR_ACCOUNT);
    
    auction.bestBid=auction.bestBidUser=0;
    setMapValue(1, 2, auction.bestBidUser);
    setMapValue(1, 3, auction.bestBid);
    setMapValue(1, 4, 0);
}

firstRun();


/*

[
  // testcase 1: regular use
  {
    // Top up the contract. Expect success and contract with balance.
    "blockheight": 2,
    "sender": "555n",
    "recipient": "999n",
    "amount": "10_0000_0000n"
  },
  {
    // Change default ad
    "blockheight": 4,
    "sender": "555n",
    "recipient": "999",
    "amount": "1_0000_0000",
    "messageText": "5tdfgd43as6s6dtst;https:/tmg.notallmine.net/"
  },
  {
    // First BID. Expect error ad not set and refund.
    "blockheight": 6,
    "sender": "1000n",
    "recipient": "999",
    "amount": "501_0000_0000",
    "messageText": "bid"
  },
  {
    // 1000 set his ad. Expect success.
    "blockheight": 8,
    "sender": "1000n",
    "recipient": "999",
    "amount": "1_0000_0000",
    "messageText": "67d7shfsdhgf:http:/deleterium.info/"
  },
  {
    // 2000 set his ad. Expect success.
    "blockheight": 8,
    "sender": "2000n",
    "recipient": "999",
    "amount": "1_0000_0000",
    "messageText": "rdda4sd4ard;https:/walter.com/"
  },
  {
    // 1000 BID. Expect to be accepted.
    "blockheight": 10,
    "sender": "1000n",
    "recipient": "999",
    "amount": "501_0000_0000",
    "messageText": "bid"
  },
  {
    // 2000 BID. Expect to be accepted. (refund user 1000)
    "blockheight": 12,
    "sender": "2000n",
    "recipient": "999",
    "amount": "511_0000_0000",
    "messageText": "bid"
  },
  {
    // End auction. Expect ad from user 2000 to be online, and new auction started
    "blockheight": 14,
    "sender": "444n",
    "recipient": "999",
    "amount": "1_1000_0000"
  }
]

[
  // Testcase 2: some wrong transactions.
  {
    // Top up the contract. Already tested.
    "blockheight": 2,
    "sender": "555n",
    "recipient": "999n",
    "amount": "10_0000_0000n"
  },
  {
    // Change default ad.  Already tested.
    "blockheight": 4,
    "sender": "555n",
    "recipient": "999",
    "amount": "1_0000_0000",
    "messageText": "5tdfgd43as6s6dtst;https:/tmg.notallmine.net/"
  },
  {
    // 1000 set his ad. Already tested.
    "blockheight": 6,
    "sender": "1000n",
    "recipient": "999",
    "amount": "1_0000_0000",
    "messageText": "67d7shfsdhgf:http:/deleterium.info/"
  },
  {
    // 1000 BID accepted. Already tested.
    "blockheight": 8,
    "sender": "1000n",
    "recipient": "999",
    "amount": "501_0000_0000",
    "messageText": "bid"
  },
  {
    // End auction. Expect ad from user 1000 to be online, and new auction started. Already tested
    "blockheight": 10,
    "sender": "444n",
    "recipient": "999",
    "amount": "1_1000_0000"
  },
  {
    // Suspend Ad. Expect update in map value.
    "blockheight": 12,
    "sender": "555n",
    "recipient": "999",
    "amount": "1_1000_0000",
    "messageText": "suspend"
  },
  {
    // Resume Ad. Expect update in map value.
    "blockheight": 14,
    "sender": "555n",
    "recipient": "999",
    "amount": "1_1000_0000",
    "messageText": "resume"
  },
  {
    // Set new minimum BID. Expect update in variables and map.
    "blockheight": 16,
    "sender": "555n",
    "recipient": "999",
    "amount": "251_0000_0000",
    "messageText": "minimum"
  },
  {
    // Set new minimum BID increment. Expect update in variables and map.
    "blockheight": 18,
    "sender": "555n",
    "recipient": "999",
    "amount": "201_0000_0000",
    "messageText": "step"
  },
  {
    // 1000 BID below minimum (but above step). Expect error.
    "blockheight": 20,
    "sender": "1000n",
    "recipient": "999",
    "amount": "205_0000_0000",
    "messageText": "bid"
  },
  {
    // 2000 set his ad. Already tested.
    "blockheight": 22,
    "sender": "2000n",
    "recipient": "999",
    "amount": "1_0000_0000",
    "messageText": "rdda4sd4ard;https:/walter.com/"
  },
  {
    // 1000 BID at minimum. Already tested.
    "blockheight": 24,
    "sender": "1000n",
    "recipient": "999",
    "amount": "251_0000_0000",
    "messageText": "bid"
  },
  {
    // 2000 BID above minimum but below step. Expect error.
    "blockheight": 26,
    "sender": "2000n",
    "recipient": "999",
    "amount": "401_0000_0000",
    "messageText": "bid"
  },
  {
    // 2000 BID above minimum and above step. Expect success.
    "blockheight": 28,
    "sender": "2000n",
    "recipient": "999",
    "amount": "451_0000_0000",
    "messageText": "bid"
  },
  {
    // 1000 BID much bigger than step. Expect success.
    "blockheight": 30,
    "sender": "1000n",
    "recipient": "999",
    "amount": "1001_0000_0000",
    "messageText": "bid"
  },
  {
    // 1000 wrong command.
    "blockheight": 32,
    "sender": "1000n",
    "recipient": "999",
    "amount": "101_0000_0000"
  },
  {
    // End auction. Expect ad from user 1000 to be online, and new auction started. Already tested
    "blockheight": 34,
    "sender": "444n",
    "recipient": "999",
    "amount": "1_1000_0000"
  },
  {
    // End auction withou bid. Expect set suspend and trigger new auction.
    "blockheight": 36,
    "sender": "444n",
    "recipient": "999",
    "amount": "1_1000_0000"
  }
]

*/