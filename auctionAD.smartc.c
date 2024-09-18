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
#define TIMER_CONTRACT 555
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
    ACTIVATE_TIMER();
    auction.minimum=MINIMUM_BID;
    auction.step=MINIMUM_BID_STEP;
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
        case 0:
            if (currentTX.sender == TIMER_CONTRACT) {
                dispatchAuctionEnd = true;
                break;
            }
            sendAmountAndMessage(currentTX.amount, message.missingCommand, currentTX.sender);
            break;
        default:
            setMapValue(3, currentTX.sender, currentTX.txId);
            sendAmountAndMessage(currentTX.amount, message.OK, currentTX.sender);
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
        setMapValue(1, 0, currentTX.amount);
        break;
    case 'step':
        setMapValue(1, 1, currentTX.amount);
        break;
    case 0:
        sendMessage(message.missingCommand, currentTX.sender);
        // Do not return the signa. Used for top up balance.
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
