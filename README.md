# Explorer AD Auction Smart Contract
## Main contract rules
* Auction ends every week. Period to be set on timer contract - 2520 blocks.
* First auction starts in the first transaction received.
* Setup message is a transaction with an unencrypted message, with signa amount greater than or equal the ACTIVATION_AMOUNT and it's content doesn't match any command. Amount above the ACTIVATION_AMOUNT will be returned.
* When Owner sends a setup message, it will be the default AD.
* When the user sends a setup a message, the contract will store it at map key1=3, key2=UserID, value=TransactionID. Contract cannot not ensure the text is valid (it would cost a lot of fee), the validation must be done at the frontend.
* Command 'bid': Anyone can send an amount to be the highest bidder.
  * If the user didn't send a setup message, do not accept his bid. (Error message: You must configure your AD)
  * Amount must be greater than or equal the minimum bid. (Error message: Your bid is too low)
  * If there was a previous bid, the amount must be greater than previous bid plus the minimun step increment. (Error message: Your bid is too low)
  * Contract will respond user with a message for successful/failed bid. (Message: Your bid was accepted)
  * If there was a previous bid, contract will respond previous user that his bid was superseded and return the bid amount. (Message: Your bid was superseeded)
* Command 'suspend': Onwer can suspend current AD
* Command 'resume': Onwer can resume current AD
* Command 'minimum': Onwer can set new minimum bid sending a value for the minimum amount. The amount will be returned by the contract.
* Command 'step': Onwer can set new step increment sending a value for the step increment amount. The amount will be returned by the contract.
* Auction end: when received a transaction without message from the timer contract:
  * If no bidder, set AD state to suspended and trigger a new auction.
  * If there is a bid:
    * Set new AD for the winner.
    * Set AD state to run again (not suspended)
    * Send a message to winner. (Message: Your AD is online!)
    * Send bid amount to Owner (small fee to contract creator).
    * Trigger a new auction.

## Timer/deploy rules
* First deploy the timer contract. Write down the timer contract ID.
* Update the main contract with the address of the timer contract.
* Deploy the main contract. Send first transaction with 10 Signa and no message.
* On first run, the main contract will send a transaction to the timer contract.
* The first transaction received by timer contract will lock it to accept activations only from the that address.
* Timer contract will sleep for the configured amount of blocks (one week), then send the wakeup command to the main contract.

## Frontend rules
For main page:
* Get '/api?requestType=getATMapValues' from key1=0.
* Set `message_transactionID` to (key2=0) value: the defauld AD.
* If the contract state (key2=2) is 0, set `message_transactionID` to (key2=1) value. Contract is not suspended.
* Get the `message_AD` from the `message_transactionID`.
* Parse the `message_AD`. If invalid, terminate (do not update).
* Change page container to show the AD found.
* Remember to add a link to the bid page.

For bid page:
* Bid page must have forms for all commands.
* Get '/api?requestType=getATMapValues' from key1=1.
* Set values accordingly for the next minimum bid.
* Parse the `current_highest_transactionID_AD`. If invalid, do not update AD preview. If Valid, draw AD preview.

## Contract memory map
Map memory key1 and key2 pairs will be refered as [key1, key2].
* [0, 0] => TransactionID for the default AD.
* [0, 1] => TransactionID for the winner's AD.
* [0, 2] => Contract state. 0 for running, 1 for suspended.
* [1, 0] => Minimum bid amount.
* [1, 1] => Minimum increment amount.
* [1, 2] => Current highest userID.
* [1, 3] => Current highest amount.
* [1, 4] => Current highest TransactionID AD (for pre-visualization purpose)
* [3, accountId] => Transaction ID with the AD details for the given account
