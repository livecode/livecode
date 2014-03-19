**Why has the API changed?**

The LiveCode engine until now supported in-app purchasing for apps distributed through the Google Play store (formerly Android Market), as well as the Apple AppStore. This support is now extended so that apps distributed through other avenues (the Amazon & Samsung app stores) can make use of the in-app purchase features provided. For this reason, new LiveCode commands have been added, and some of the old ones have slightly changed. However, all of the old commands are still supported (for the Google Play Store and the Apple AppStore). In order the existing scripts users have written to continue to work, all it needs is to add one or two extra lines, depending on the store. More details on this later. Moreover, the new API allows the user to query specific product information (such as price, description etc) before they make a purchase, and supports purchasing of subscription items for all available stores. Furthermore, for the Google Play Store, the new API uses the newest version of Google In-App Billing API (v3), that offers synchronous purchase flow, and purchase information is available immediately after it completes. This information of in-app purchases is maintained within the Google Play system until the purchase is _consumed._ More on the consumption of purchased items later.

**What has changed? **

To start with, the main changes are the following:

1. Each item has an extra property, the _itemType_, that has to be specified before making a purchase. This is done using the **mobileStoreSetProductType** command. The _itemType_ can either be _subs_, for subscription items, or _inapp_ for consumable and non-consumable items.

2. Due to a restriction of the newest version of Google In-App Billing API, you cannot buy consumable items more than once, unless you consume them. This is done using the **mobileStoreConsumePurchase** command. Note that this command is actually only used when interacting with the Google Play Store API. What it does is sending a consumption request to Google Play, so that you will be able to buy this product again. You would typically implement consumption for items that can be purchased multiple times (i.e. for consumable products, such as in-game currency, fuel etc). Note that in case you call mobileStoreConsumePurchase on a non consumable product, then you no longer own this item.

3. The new purchase flow has become simpler. 
Instead of

    - creating a purchase request (**mobilePurchaseCreate** productID)

    - store the new purchase request ID (put the result into tPurchaseID),

    - setting properties such as quantity and developer payload (**mobilePurchaseSet** tPurchaseID, “quantity”, pQuantity)

    - sending a purchase request to the store (**mobilePurchaseSendRequest** tPurchaseID)

    now all it needs is just

    - set the product type (**mobileStoreSetProductType** productID, itemType)

    - make a purchase (**mobileStoreMakePurchase** productID, quantity, developerPayload)


4. The **purchaseStateUpdate** message that the store sends in response to **mobileStoreMakePurchase**, contains not only the purchase identifier and the state of the purchase, but also the product identifier of the requested item:

     **purchaseStateUpdate** _purchaseID_, _productID_, _state_

5. So you can query a purchased product property using the product identifier, instead of the purchase identifier:

    **mobileStoreProductProperty** _productID_, _propertyName_

     Note that the old function **mobileGetPurchase** _purchaseID_, _propertyName_ will still work.

6. You can get information on a specific item (such as product identifier, product type, price etc), using the **mobileStoreRequestProductDetails** command. The store responds:

     In case the request is successful, a **productDetailsReceived** message is sent by the store.

     In case of failure, a **productRequestError** message is sent by the store.

7. You can get a list of all known completed purchases using **mobileStorePurchasedProducts** function. This returns a list of product identifiers of restored or newly bought purchases.

**What needs to change in existing scripts?**

It is recommended that scripts which were written using previous versions of LiveCode (and thus use the old LiveCode API for in-app purchasing), should be used to run on these versions. However, it is still possible to run an existing script (that makes use of in-app purchasing feature) on LiveCode 6.7, only by changing a few things:

- **purchaseStateUpdate** message is now called with 3 parameters, (purchaseID, productID, state), instead of two (purchaseID, state). This applies to apps built for both the Google Play Store and the Apple AppStore.

- before sending a **mobilePurchaseSendRequest**, you have to specify the type (_subs_ or _inapp_) of the item using **mobileStoreSetProductType** _productID_, _type_ command (Google Play Store only).

- if you want to buy more than one consumable item, you have to consume it first. This can be done by using the **mobileStoreConsumePurchase** _productID_ command (Google Play Store only).

If you want to build apps for Amazon and/or Samsung Store, you have to use the newest LiveCode API.  

**How to use the new API?**

**Setup**

Before you can use IAP, you must set up products in each vendor’s developer portal. In general terms, you have to:

- Create each product you want to sell, giving it a unique identifier. Note that for the Samsung Seller Office, the developer cannot choose the product identifier. This is assigned by the store.
- Submit the items for approval to the appropriate store. Some stores may require additional metadata, such as screenshots of your for sale items.
- Set up unique test accounts. The user is not charged when making a purchase using the test account details. This applies to Apple and Google. Amazon and Samsung have different methods for testing.

For more detailed store-specific information, you can have a look at the links below:

[Apple AppStore](https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnectInAppPurchase_Guide/Chapters/Introduction.html#//apple_ref/doc/uid/TP40013727-CH1-SW1)

[Google Play Store](http://developer.android.com/google/play/billing/billing_admin.html)

[Amazon Appstore](https://developer.amazon.com/public/apis/earn/in-app-purchasing/docs/submitting)

[Samsung Apps Store](http://developer.samsung.com/in-app-purchase) and more specifically click [here](http://img-developer.samsung.com/contents/cmm/EN_Samsung_IAP2.0_Programming_Guide_Overview_v1.0.pdf)

**Purchase Types**

  There are three classes of products users can purchase:
  
1.  One-time purchases that get “consumed”. Typically, these items are called _consumables_. The user can buy as many times as they want (virtual coins/bullets in a game), except in apps built for the Google Play Store, where the user has to consume the purchased item first, and then buy (one) more.	
2.	One-time purchases that last forever, such as unlocking extra features, downloading new content once. These items are usually called _non-consumables_.
3.	Subscriptions where the app user pays a periodical fee to receive some ongoing service. Subscriptions can either be auto-renewable or non-renewable. 

Each vendor uses different terminology for these purchases : 

|        | Apple           | Google  |Amazon       |Samsung  |
| ------------- |:-------------:| :-----:|:----------:|:-----:|
|   one-time, gets consumed    | consumable | unmanaged |consumable       |consumable  |
| one-time, lasts forever     | non-consumable      |   managed |entitlement       |non-consumable  |
| subscriptions | auto-renewable , non-renewable  |    auto-renewable |auto-renewable       |non-renewable  |


**Testing**

Again, each store uses a different method of testing.

For the Apple AppStore, you can create test accounts. More details [here](https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnectInAppPurchase_Guide/Chapters/TestingInAppPurchases.html#//apple_ref/doc/uid/TP40013727-CH4-SW1)

For the Google Play Store, you can create test accounts as well as test using static responses. More details [here](http://developer.android.com/google/play/billing/billing_testing.html). Note that you cannot test subscriptions using the test account. This means that the test user will be charged when purchasing a subscription item. A possible workaround to this, is to log into the Google Wallet Service as a seller, using your Google Developer account details, and “refund” and then “cancel” the order of the subscription item that the test user had just purchased.

For the Amazon Appstore, you can test your app using SDK Tester. This is a developer tool that allows users of the Amazon Mobile App SDK to test their implementation in a production-like environment before submitting it to Amazon for publication. More details [here](https://developer.amazon.com/public/apis/earn/in-app-purchasing/docs/testing-iap)

For the Samsung Apps Store, Samsung IAP API offers three modes to test the service under various conditions : _Production Mode, Test Mode Success, Test Mode Fail_. During development period, you can select the mode in the Standalone Application Settings window. Before releasing your application, you must change to Production Mode. If you release your application in Test Mode, actual payments will not occur. More details on page 6 and 7 [here](http://img-developer.samsung.com/contents/cmm/EN_Samsung_IAP2.0_Android_Programming_Guide_v1.0.pdf)

Note that in Production Mode, your app can only interact with item groups with _sales_ status. This information exists in the Samsung Seller Office. However, item groups are only given sales status after the app has been certified. In other words, you can test your app in Production Mode only after it has been certified by Samsung.

**Syntax**

Implementing in-app purchasing requires two way communication between your LiveCode app and the vendor’s store. Here is the basic process:

- Your app sends a request to purchase a specific in-app purchase to the store
- The store verifies this and attempts to take payment
- If payment is successful the store notifies your app
- Your app unlocks features or downloads new content / fulfils the in-app purchase
- Your app tells the store that all actions associated with the purchase have been completed
- Store logs that in-app purchase has been completed

**Commands, Functions and Messages**

To determine if in-app purchasing is available use:

**mobileStoreCanMakePurchase()**

Returns _true_ if in-app purchases can be made, _false_ if not.

Throughout the purchase process, the store sends **purchaseStateUpdate** messages to your app which report any changes in the status of active purchases. The receipt of these messages can be switched on and off using:

**mobileStoreEnablePurchaseUpdates**  
**mobileStoreDisablePurchaseUpdates**

If you want to get information on a specific item (such as product identifier, product type, price etc), you can use:

**mobileStoreRequestProductDetails** _productID_

The _productID_ is the identifier of the item you are interested. Then, the store sends a _productDetailsReceived_ message, in case the request is successful, otherwise it sends a _productRequestError_ message:

**productDetailsReceived** _productID_, _details_

The _productID_ is the identifier of the item, and _details_ is an array with the following keys - that are different depending on the store:

For Android stores (Google, Amazon, Samsung), the keys are:

- _productID_ – identifier of the requested product
- _price_ – price of the requested product
- _description_ –  description of the requested product
- _title_ – title of the requested product
- _itemType_ – type of the requested product
- _itemImageUrl_ – URL where the image (if any) of the requested product is stored
- _itemDownloadUrl_ – URL to download the requested product
- _subscriptionDurationUnit_ — subscription duration unit of the requested product
- _subscriptionDurationMultiplier_ — subscription duration multiplier of the requested product

Note that some Android stores do not provide values for all the above keys. In this case, the value for the corresponding key will be empty.

For iTunes Connect store (Apple), the keys of _details_ array are the following:

- _price_ – price of the requested product
- _description_ –  description of the requested product
- _title_ – title of the requested product
- _currency code_ – price currency code of the requested product
- _currency symbol_ — currency symbol of the requested product
- _unicode description_ — unicode description of the requested product
- _unicode title_ — unicode title of the requested product
- _unicode currency symbol_ — unicode currency symbol of the requested product

If **mobileStoreRequestProductDetails** is not successful, then a _productRequestError_ message is sent :

**productRequestError** _productID_, _error_

The _productID_ is the identifier of the item, and _error_ is a string that describes the error.

Before sending a purchase request for a particular item, you have to specify the type of this item. To do this, use :

**mobileStoreSetProductType** _itemType_

The _itemType_ can either be _subs_ or _inapp_.

To create and send a request for a new purchase use:

**mobileStoreMakePurchase** _productID_, _quantity_, _developerPayload_

The _productID_ is the identifier of the in-app purchase you created in the vendor’s developer portal and wish to purchase. The _quantity_ specifies the quantity of the in-app purchase to buy (iOS only - always “1” in Android)) . The _developerPayload_ is a string of less than 256 characters that will be returned with the purchase details once complete. Can be used to later identify a purchase response to a specific request (Android only).


To get a list of all known completed purchases use:

**mobileStorePurchasedProducts()**

It returns a return-separated list of product identifiers, of restored or newly bought purchases which are confirmed as complete. Note that in iOS, consumable products as well as non-renewable subscriptions will not be contained in this list.

Once a purchase is complete, you can retrieve the properties of the purchased product, using:

**mobileStoreProductProperty** _(productID, property)_

The parameters are as follows:

- _productID_ – identifier of the requested product
- _property_ – name of the purchase request property to get

Properties which can be queried can differ depending on the store: 

For the Samsung Apps Store (Android), you can query the properties:

- _title_– title of the purchased product
- _productId_ – identifier of the purchased product
- _price_ – price of the purchased product
- _currencyUnit_ – currency unit of the product price
- _description_ – description of the product as specified in the Samsung Seller Office
- _itemImageUrl_ — URL where the image of the purchased product is stored
- _itemDownLoadUrl_ — URL to download the purchased product 
- _paymentId_ — payment identifier of the purchased product
- _purchaseId_ — purchase identifier of the purchased product
- _purchaseDate_ — purchase date, in milliseconds
- _verifyUrl_ — IAP server URL for checking if the purchase is valid for the IAP server, using the _purchaseId_ value

For the Google Play Store (Android), you can query the properties:

- _productId_ – identifier of the purchased product
- _packageName_ – application package from which the purchase originated
- _orderId_ – unique order identifier for the transaction. This corresponds to the Google Wallet Order ID
- _purchaseTime_ – time the product was purchased, in milliseconds
- _developerPayload_– developer-specified string that contains supplemental information about an order. You can specify a value for this in **mobileStoreMakePurchase**
- _purchaseToken_– token that uniquely identifies a purchase for a given item and user pair.
- _itemType_ — type of the purchased item, _inapp_ or _subs_
- _signature_ — string containing the signature of the purchase data that was signed with the private key of the developer. The data signature uses the RSASSA-PKCS1-v1_5 scheme

For the Amazon Appstore (Android), you can query the properties:

- _productId_ – identifier of the purchased product
- _itemType_ — type of the purchased product. This can be _CONSUMABLE_, _ENTITLED_ or _SUBSCRIPTION_
- _subscriptionPeriod_ – string indicating the start and end date for subscription (for subscription products only)
- _purchaseToken_– purchase token that can be used from an external server to validate purchase

For Apple AppStore (iOS), you can query the properties:

- _quantity_ – amount of item purchased. You can specify a value for this in **mobileStoreMakePurchase**
- _productId_ – identifier of the purchased product
- _receipt_ – block of data that can be used to confirm the purchase from a remote server with the iTunes Connect store 
- _purchaseDate_ – date the purchase / restoration request was sent
- _transactionIdentifier_ – unique identifier for a successful purchase / restoration request
- _originalPurchaseDate_ –  date of the original purchase, for restored purchases 
- _originalTransactionIdentifier_ – the transaction identifier of the original purchase, for restored purchases 
- _originalReceipt_ – the receipt for the original purchase, for restored purchases  

Once you have sent your purchase request and it has been confirmed, you can then unlock or download new content to fulfil the requirements of the in-app purchase. You must inform the store once you have completely fulfiled the purchase using:

**mobileStoreConfirmPurchase** _productID_

Here, _productID_ is the identifier of the product requested for purchase.

**mobileStoreConfirmPurchase** should only be called on a purchase request in the _paymentReceived_ or _restored_ state (more on the states of the purchase later). If you don’t send this confirmation before the app is closed, **purchaseStateUpdate** messages for the purchase will be sent to your app the next time updates are enabled by calling the **mobileStoreEnablePurchaseUpdates** command.

To consume a purchased product use:

**mobileStoreConsumePurchase** _productID_

Here, _productID_ is the identifier of the product requested for consumption. Note that this command is actually only used when interacting with the Google Play Store API. This is because the Google Play Store API has a restriction that ensures a consumable product is consumed before another instance is purchased. _Consume_ means that the purchase is removed from the user's inventory of purchased items, allowing the user buy that product again. 

Note that **mobileStoreConsumePurchase** must only be called on consumable products. If you call **mobileStoreConsumePurchase** on a non-consumable product, then you no longer own this product. 

To instruct the store to re-send notifications of previously completed purchases use:

**mobileStoreRestorePurchases**

This would typically be called the first time an app is run after installation on a new device to restore any items bought through the app.

To get more detailed information about errors in the purchase request use:

**mobileStorePurchaseError** _(purchaseID)_


The store sends **purchaseStateUpdate** messages to notifies your app of any changes in state to the purchase request. These messages continue until you notify the store that the purchase is complete or it is cancelled.

**purchaseStateUpdate** _purchaseID, productID, state_

The state can be any one of the following:

- _sendingRequest_ – the purchase request is being sent to the store / marketplace
- _paymentReceived_ – the requested item has been paid for. The item should now be delivered to the user and confirmed via the mobileStoreConfirmPurchase command
- _alreadyEntitled_ – the requested item is already owned, and cannot be purchased again
- _invalidSKU_ – the requested item does not exist in the store listing
- _complete_ – the purchase has now been paid for and delivered
- _restored_ – the purchase has been restored after a call to mobileStoreRestorePurchases. The purchase should now be delivered to the user and confirmed via the mobileStoreConfirmPurchase command
- _cancelled_ – the purchase was cancelled by the user before payment was received
- _error_ – An error occurred during the payment request. More detailed information is available from the mobileStorePurchaseError function