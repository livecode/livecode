**Why has the API changed?**

The Livecode engine until now supported in-app purchasing for apps distributed through the Google Play store (formerly Android Market), as well as the Apple AppStore. This support is now extended so that apps distributed through other avenues (the Amazon & Samsung app stores) can make use of the in-app purchase features provided. For this reason, new Livecode commands have been added, and some of the old ones have slightly changed. However, all of the old commands are still supported (for GooglePlay Store and Apple AppStore). In order the existing scripts users have written to continue to work, all it needs is to add one or two extra lines, depending on the store. More details on this later. Moreover, the new API allows the user to query for specific product information (such as price, description etc) before she makes a purchase, and supports purchasing of subscription items for all available stores. Furthermore, for Google Play Store, the new API uses the newest version of Google In-App Billing API (v3), that offers synchronous purchase flow, and purchase information is available immediately after it completes. This information (of in-app purchases) is maintained within the Google Play system until the purchase is _consumed._ More on the consumption of purchased items later.

**What has changed? **

To start with, the main changes are the following:

1. Each item has an extra property, the _itemType_, that has to be specified before making a purchase. This is done using the **mobileProductSetType** _itemType_ command. The _itemType_ can be either _subs_ (for subscription items) or _inapp_ (for consumable and non-consumable items).

2. Due to a restriction of  the newest version of Google In-App Billing API, you cannot buy consumable items more than once, unless you consume them. This is done using the **mobileConsumePurchase _productID_**. Here, _productID_ is the identifier of the product requested for consumption. Note that this command is practically used only when interacting with Google Play Store API. What it does issending a consumption request to Google Play, so that you will be able to buy this product again. You would typically implement consumption for items that can be purchased multiple times (i.e. for consumable products, such as in-game currency, fuel etc). Note that in case you call mobileConsumePurchase on a non consumable product, you will no longer own this item (i.e you would have to buy it again).

3. The new purchase flow has become simpler. 
Instead of

    - creating a purchase request (**mobilePurchaseCreate** productID)

    - store the new purchase request ID (put the result into tPurchaseID),

    - setting properties such as quantity and developer payload (**mobilePurchaseSet** tPurchaseID, “quantity”, pQuantity)

    - sending a purchase request to the store (**mobilePurchaseSendRequest** tPurchaseID)

    now all it needs is just

    - set the product type (**mobileProductSetType** productID, itemType)

    - make a purchase (**mobileMakePurchase** productID, quantity, developerPayload)


4. The **purchaseStateUpdate** message that the store sends in response to **mobileMakePurchase**, contains not only the purchaseID and the state of the purchase, but also the productID of the requested item:

     **purchaseStateUpdate** purchaseID, productID, state

5. So you can query for a purchase property using the productID, instead of the purchaseID:

    **mobilePurchaseGetProperty** productID, propertyName

     Note that the old command **mobileGetPurchase** purchaseID, propertyName will still work.

6. You can get information on a specific item (such as product id, product type, price etc), using **mobileRequestProductDetails** productID. The store responds:

     In case the request is successful, a **productDetailsReceived** productId, details message is sent by the store.

     In case of failure, a **productRequestError** productId, error message is sent by the store.

7. You can get a list of all known completed purchases using **mobileGetPurchases**. This returns a list of product identifiers of restored or newly bought purchases.

**What needs to change in existing scripts?**

It is recommended that scripts which were written using previous versions of Livecode (and thus use the old Livecode API for in-app purchasing), should be used to run on these versions. However, it is still possible to run an existing script (that makes use of in-app purchasing feature) on Livecode 6.6, only by changing a few things:

- **purchaseStateUpdate** message is now returned with (purchaseID, productID, state),instead of(purchaseID, state). This applies to apps built for both Google Play Store and Apple AppStore.

- before sending a **mobilePurchaseSendRequest**, you have to specify the type (subs or inapp) of the item using **mobileProductSetType** productID, type command (Google Play Store only).

- if you want to buy more than one consumable item, you have to consume it first. This can be done by calling  **mobileConsumePurchase productID**  through a “consume itemX” button (Google Play Store only).

If you want to build apps for Amazon and/or Samsung Store, you have to use the newest API Livecode commands. 

**How to use the new API?**

**Setup**

Before you can use IAP, you must set up products in each vendor’s developer portal. In general terms, you have to:

- Create each product you want to sell, giving it a unique ID. (Note : In Samsung Seller Office, the user *cannot* choose the product ID. This is generated by the store.
- Submit the items for approval to the appropriate store. Some stores may require additional metadata, such as screenshots of your for sale screens.
- Set up unique test accounts, i.e accounts that the user is not charged when making a purchase (for Apple and Google).  Amazon and Samsung have a different method of testing.

For more detailed store-specific information, one can have a look at the links below :

iTunes Store: [https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnectInAppPurchase_Guide/Chapters/Introduction.html#//apple_ref/doc/uid/TP40013727-CH1-SW1](https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnectInAppPurchase_Guide/Chapters/Introduction.html#//apple_ref/doc/uid/TP40013727-CH1-SW1)

Google Play Store: [http://developer.android.com/google/play/billing/billing_admin.html](http://developer.android.com/google/play/billing/billing_admin.html)

Amazon Store: [https://developer.amazon.com/public/apis/earn/in-app-purchasing/docs/submitting](https://developer.amazon.com/public/apis/earn/in-app-purchasing/docs/submitting)

Samsung Store: [http://developer.samsung.com/in-app-purchase](http://developer.samsung.com/in-app-purchase) and more specifically [http://img-developer.samsung.com/contents/cmm/EN_Samsung_IAP2.0_Programming_Guide_Overview_v1.0.pdf](http://img-developer.samsung.com/contents/cmm/EN_Samsung_IAP2.0_Programming_Guide_Overview_v1.0.pdf)

**Purchase Types**

  There are generally three classes of things users can purchase:
  
1.  One-time purchases that get “consumed”. Typically, these items are called _consumables_. The user can buy as many times as she wants (virtual coins/bullets in a game), with an exception for apps built for Google Play Store, where the user has to consume the purchased item first, and then buy (one) more.	
2.	One-time purchases that last forever, (such as unlocking extra features, downloading new content once). These items are usually called _non-consumables_.
3.	Subscriptions where the app user pays a (monthly/yearly) fee to receive some ongoing service. Subscriptions can be either auto-renewable or non-renewable. 

Each vendor uses different terminology for these purchases : 

|        | Apple           | Google  |Amazon       |Samsung  |
| ------------- |:-------------:| :-----:|:----------:|:-----:|
|   one-time, gets consumed    | consumable | unmanaged |consumable       |consumable  |
| one-time, lasts forever     | non-consumable      |   managed |entitlement       |non-consumable  |
| subscriptions | auto-renewable , non-renewable  |    auto-renewable |auto-renewable       |non-renewable  |


**Testing**

Again, each store uses a different method of testing.

For iTunes Store, you can create test accounts. More details on [https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnectInAppPurchase_Guide/Chapters/TestingInAppPurchases.html#//apple_ref/doc/uid/TP40013727-CH4-SW1](https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnectInAppPurchase_Guide/Chapters/TestingInAppPurchases.html#//apple_ref/doc/uid/TP40013727-CH4-SW1)

For Google Play Store, you can create test accounts as well as test using static responses. More details on [http://developer.android.com/google/play/billing/billing_testing.html](http://developer.android.com/google/play/billing/billing_testing.html) (Note : you cannot test subscriptions using the test account. This means that the test user WILL be charged when purchasing a subscription item. A possible workaround to this, is to log into Google Wallet Service (as a seller, using your Google Developer account details), and “refund” and then “cancel” the order of the subscription item the test user had just purchased.

For Amazon App Store, you can test your app using SDK Tester. This is a developer tool that allows users of the Amazon Mobile App SDK to test their implementation in a production-like environment before submitting it to Amazon for publication. More details on [https://developer.amazon.com/public/apis/earn/in-app-purchasing/docs/testing-iap](https://developer.amazon.com/public/apis/earn/in-app-purchasing/docs/testing-iap)

For Samsung App Store, Samsung IAP API offers three modes to test the service under various conditions : Production Mode, Test Mode Success, Test Mode Fail. During development period, you can select the mode in the Standalone Application Settings tab. Before releasing your application, you must change to Production Mode. If you release your application in Test Mode, actual payments will not occur. More details on page 6 and 7 : [http://img-developer.samsung.com/contents/cmm/EN_Samsung_IAP2.0_Android_Programming_Guide_v1.0.pdf](http://img-developer.samsung.com/contents/cmm/EN_Samsung_IAP2.0_Android_Programming_Guide_v1.0.pdf)

Important : In Production Mode, your app can only interact with item groups with _sales_ status (this info exists in Samsung Seller Office). However, item groups are only given sales status AFTER the app has been certified. In a few words, you can test your app in Production Mode only after it has been certified by Samsung.

**Syntax**

Implementing in-app purchasing requires two way communication between your LiveCode app and the vendor’s store. Here is the basic process:

- Your app sends a request to purchase a specific in-app purchase to the store
- The store verifies this and attempts to take payment
- If payment is successful the store notifies your app
- Your app unlocks features or downloads new content / fulfills the in-app purchase
- Your app tells the store that all actions associated with the purchase have been completed
- Store logs that in-app purchase has been completed

**Commands and Functions**

To determine if in app purchasing is available use:

**mobileCanMakePurchase()**

Returns _true_ if in-app purchases can be made, _false_ if not.

Throughout the purchase process, the AppStore sends **purchaseStateUpdate** messages to your app which report any changes in the status of active purchases. The receipt of these messages can be switched on and off using:

**mobileEnablePurchaseUpdates**  
**mobileDisablePurchaseUpdates**

If you want to get information on a specific item (such as product id, product type, price etc), you can use:

**mobileRequestProductDetails** _productId_

The _productId_ is the identifier of the item you are interested. Then, the store sends a _productDetailsReceived_ message, in case the request is successful, otherwise it sends a _productRequestError_ message:

**productDetailsReceived** _productId_, _details_

The _productId_ is the identifier of the item, and _details_ is an array with the following keys (slightly different depending on the store):

For Android stores (Google, Amazon, Samsung), the keys are:

- _productID_– the identifier of the requested product
- _price_ – price of requested item
- _description_ –  description of requested item
- _title_ – title of requested item
- _itemType_ – type of requested item
- _itemImageUrl_ – the URL where the image of the requested item is stored
- _itemDownloadUrl_ – the URL to download the requested item
- _subscriptionDurationUnit_ — subscription duration unit of requested item
- _subscriptionDurationMultiplier_ — subscription duration multiplier of requested item

Note that some Android stores do not provide values for all the above keys. In this case, the value for the corresponding key will be empty.

For iTunes Connect store (Apple), the keys of _details_ array are the following:

- _price_ – price of requested item
- _description_ –  description of requested item
- _title_ – title of requested item
- _currency code_ – price currency code of requested item
- _currency symbol_ — currency symbol of requested item
- _unicode description_ — unicode description of requested item
- _unicode title_ — unicode title of requested item
- _unicode currency symbol_ — unicode currency symbol of requested item

If **mobileRequestProductDetails** is not successful, then a _productRequestError_ message is sent :

**productRequestError** _productId_, _error_

The _productId_ is the identifier of the item, and _error_ is a string that describes the error.

Before sending a purchase request for a particular item, you have to specify the type of this item. To do this, use :

**mobileProductSetType** _itemType_

The _itemType_ can be either _subs_ or _inapp_.

To create and send a request for a new purchase use:

**mobileMakePurchase** _productID_, _quantity_, _developerPayload_

The _productID_ is the identifier of the in-app purchase you created in iTunesConnect and wish to purchase. The _quantity_ specifies the quantity of the in-app purchase to buy (default 1) (iOS only) . The _developerPayload_ is a string of less than 256 characters that will be returned with the purchase details once complete. Can be used to later identify a purchase response to a specific request. Defaults to empty (Android only).

To query the status of an active purchase use:

**mobilePurchaseState** (_purchaseID_)

The _purchaseID_ is the identifier of the purchase request, and is returned in **purchaseStateUpdate** message (more information on this below). One of the following is returned

- _initialized_ – the purchase request has been created but not sent. In this state additional properties such as the item quantity can be set. (Available only in v1 in-app purchase API)
- _sendingRequest_ – the purchase request is being sent to the store / marketplace.
- _paymentReceived_ – the requested item has been paid for. The item should now be delivered to the user and confirmed via the **mobileConfirmPurchase** command.
- _alreadyEntitled_ – the requested item is already owned, and cannot be purchased again
- _invalidSKU_ – the requested item does not exist in the store listing
- _complete_ – the purchase has now been paid for and delivered
- _restored_ – the purchase has been restored after a call to mobileRestorePurchases. The purchase should now be delivered to the user and confirmed via the mobilePurchaseConfirmDelivery command.
- _cancelled_ – the purchase was cancelled by the user before payment was received
- _error_ – An error occurred during the payment request. More detailed information is available from the mobilePurchaseError function

To get a list of all known active purchases use:

**mobilePurchases()**

It returns a return-separated list of purchase identifiers, of restored or newly bought purchases which have yet to be confirmed as complete.

To get a list of all known completed purchases use:

**mobileGetPurchases()**

It returns a return-separated list of product identifiers, of restored or newly bought purchases which are confirmed as complete. Note that in iOS, consumable products as well as non-renewable subscriptions will not be contained in this list.

Once a purchase is complete, you can retrieve its properties, using:

**mobileGetPurchaseProperty** _(productID, property)_

The parameters are as follows:

- _productID_ – the identifier of the requested product
- _property_ – the name of the purchase request property to get

Properties which can be queried vary depending on the store: 

For Samsung Store (Android), you can query for the properties:

- _title_– title of item purchased 
- _productId_ – identifier of the purchased product
- _price_ – price of item purchased
- _currencyUnit_ – the currency unit of the item’s price
- _description_ – the description of the item as specified in Samsung Seller Office
- _itemImageUrl_ — the URL where the image of the purchased item is stored
- _itemDownLoadUrl_ — the URL to download the purchased item 
- _paymentId_ — payment ID of the purchased item
- _purchaseId_ — purchase ID of the purchased item
- _purchaseDate_ — purchase date (in milliseconds)
- _verifyUrl_ — IAP server URL for checking if the purchase is valid for the IAP server, using the purchaseId value

For Google Play Store (Android), you can query for the properties:

- _productId_ – identifier of the purchased product
- _packageName_ – The application package from which the purchase originated
- _orderId_ – A unique order identifier for the transaction. This corresponds to the Google Wallet Order ID
- _purchaseTime_ – The time the product was purchased, in milliseconds since the epoch (Jan 1, 1970)
- _developerPayload_– A developer-specified string that contains supplemental information about an order. You can specify a value for this in **mobileMakePurchase.**
- _purchaseToken_– A token that uniquely identifies a purchase for a given item and user pair.
- _itemType_ — The type of the purchased item (“inapp” or “subs”)
- _signature_ — String containing the signature of the purchase data that was signed with the private key of the developer. The data signature uses the RSASSA-PKCS1-v1_5 scheme.

For Amazon Store (Android), you can query for the properties:

- _productId_ – the identifier of the purchased product
- _itemType_ — the type of the purchased item (“CONSUMABLE”, “ENTITLED” or “SUBSCRIPTION”)
- _subscriptionPeriod_ – a string indicating the start and end date for subscription (only for itemType = “SUBSCRIPTION”)
- _purchaseToken_– A purchase token that can be used from an external server to validate purchase.

For iTunes Store (iOS), you can query for the properties:

- _quantity_ – amount of item purchased. You can specify a value for this in **mobileMakePurchase**
- _productId_ – identifier of the purchased product
- _receipt_ – block of data that can be used to confirm the purchase from a remote server with the itunes store 
- _purchaseDate_ – date the purchase / restore request was sent
- _transactionIdentifier_ – the unique identifier for a successful purchase / restore
- _originalPurchaseDate_ – for restored purchases – date of the original purchase 
- _originalTransactionIdentifier_ – for restored purchases – the transaction identifier of the original purchase 
- _originalReceipt_ – for restored purchases – the receipt for the original purchase 

Once you have sent your purchase request and it has been confirmed you can then unlock or download new content to fulfill the requirements of the in-app purchase. You must inform the AppStore once you have completely fulfilled the purchase using:

**mobileConfirmPurchase** _productID_

Here, _productID_ is the identifier of the product requested for purchase.

**mobileConfirmPurchase** should only be called on a purchase request in the ‘paymentReceived’ or ‘restored’ state. If you don’t send this confirmation before the app is closed, **purchaseStateUpdate** messages for the purchase will be sent to your app the next time updates are enabled by calling the mobileEnableUpdates command.

To consume a purchased item use:

**mobileConsumePurchase** _productID_

Here, _productID_ is the identifier of the product requested for purchase. Note that this command is practically used only when interacting with Google Play Store API. This is because Google Play Store API has the restriction that you cannot buy more than one consumable product, unless you “consume” it first. What “consume’ means, is that the purchase is removed from the user’s “inventory” of purchased items, so that the user can buy it again. 

IMPORTANT : Note that **mobileConsumePurchase** should be called *only* on consumable items. In case you call **mobileConsumePurchase** on a non consumable item’s _productId_, you will no longer own this item. 

To instruct the AppStore to re-send notifications of previously completed purchases use:

**mobileRestorePurchases**

This would typically be called the first time an app is run after installation on a new device to restore any items bought through the app.

To get more detailed information about errors in the purchase request use:

**mobilePurchaseError** _(purchaseID)_

**Messages**

The AppStore sends **purchaseStateUpdate** messages to notifies your app of any changes in state to the purchase request. These messages continue until you notify the AppStore that the purchase is complete or it is cancelled.

**purchaseStateUpdate** _purchaseID, productID, state_

The state can be any one of the following:

- _initialized_ – the purchase request has been created but not sent. In this state additional properties such as the item quantity can be set.
- _sendingRequest_ – the purchase request is being sent to the store / marketplace
- _paymentReceived_ – the requested item has been paid for. The item should now be delivered to the user and confirmed via the mobilePurchaseConfirmDelivery command
- _alreadyEntitled_ – the requested item is already owned, and cannot be purchased again
- _invalidSKU_ – the requested item does not exist in the store listing
- _complete_ – the purchase has now been paid for and delivered
- _restored_ – the purchase has been restored after a call to mobileRestorePurchases. The purchase should now be delivered to the user and confirmed via the mobilePurchaseConfirmDelivery command
- _cancelled_ – the purchase was cancelled by the user before payment was received
- _error_ – An error occurred during the payment request. More detailed information is available from the mobilePurchaseError function