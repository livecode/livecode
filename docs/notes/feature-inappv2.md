# **In-App Purchasing

**Why has the API changed?**

The LiveCode engine until now supported in-app purchasing for apps distributed through the Google Play store (formerly Android Market), as well as the Apple AppStore. This support is now extended so that apps distributed through other avenues (the Amazon & Samsung app stores) can make use of the in-app purchase features provided. For this reason, new LiveCode commands have been added, and some of the old ones have slightly changed. However, all of the old commands are still supported (for the Google Play Store and the Apple AppStore). In order the existing scripts users have written to continue to work, all it needs is to add one or two extra lines, depending on the store. More details on this later. Moreover, the new API allows the user to query specific product information (such as price, description etc) before they make a purchase, and supports purchasing of subscription items for all available stores. Furthermore, for the Google Play Store, the new API uses the newest version of Google In-App Billing API (v3), that offers synchronous purchase flow, and purchase information is available immediately after it completes. This information of in-app purchases is maintained within the Google Play system until the purchase is *consumed*. More on the consumption of purchased items later.

**What has changed?**

To start with, the main changes are the following:

1.  Each item has an extra property, the *itemType*, that has to be specified before making a purchase. This is done using the **mobileStoreSetProductType** command. The *itemType* can either be *subs*, for subscription items, or *inapp* for consumable and non-consumable items.

2.  Due to a restriction of the newest version of Google In-App Billing API, you cannot buy consumable items more than once, unless you consume them. This is done using the **mobileStoreConsumePurchase** command. Note that this command is actually only used when interacting with the Google Play Store API. What it does is sending a consumption request to Google Play, so that you will be able to buy this product again. You would typically implement consumption for items that can be purchased multiple times (i.e. for consumable products, such as in-game currency, fuel etc). Note that in case you call mobileStoreConsumePurchase on a non consumable product, then you no longer own this item.

3.  The new purchase flow has become simpler. 
	Instead of

	- creating a purchase request (**mobilePurchaseCreate** productID)

	- store the new purchase request ID (put the result into tPurchaseID),

	- setting properties such as quantity and developer payload (**mobilePurchaseSet** tPurchaseID, "quantity", pQuantity)

	- sending a purchase request to the store (**mobilePurchaseSendRequest** tPurchaseID)

	now all it needs is just

	- set the product type (**mobileStoreSetProductType** productID, itemType)

	- make a purchase (**mobileStoreMakePurchase** productID, quantity, developerPayload)


4.  The **purchaseStateUpdate** message that the store sends in response to **mobileStoreMakePurchase**, contains not only the purchase identifier and the state of the purchase, but also the product identifier of the requested item:

	**purchaseStateUpdate** *purchaseID*, *productID*, *state*

5.  So you can query a purchased product property using the product identifier, instead of the purchase identifier:

	**mobileStoreProductProperty** *productID*, *propertyName*

	Note that the old function **mobileGetPurchase** *purchaseID*, *propertyName* will still work.

6.  You can get information on a specific item (such as product identifier, product type, price etc), using the **mobileStoreRequestProductDetails** command. The store responds:

	In case the request is successful, a **productDetailsReceived** message is sent by the store.

	In case of failure, a **productRequestError** message is sent by the store.

7.  You can get a list of all known completed purchases using **mobileStorePurchasedProducts** function. This returns a list of product identifiers of restored or newly bought purchases.

**What needs to change in existing scripts?**

It is recommended that scripts which were written using previous versions of LiveCode (and thus use the old LiveCode API for in-app purchasing), should be used to run on these versions. However, it is still possible to run an existing script (that makes use of in-app purchasing feature) on LiveCode 6.7, only by changing a few things:

- **purchaseStateUpdate** message is now called with 3 parameters, (purchaseID, productID, state), instead of two (purchaseID, state). This applies to apps built for both the Google Play Store and the Apple AppStore.

- before sending a **mobilePurchaseSendRequest**, you have to specify the type (*subs* or *inapp*) of the item using **mobileStoreSetProductType** *productID*, *type* command (Google Play Store only).

- if you want to buy more than one consumable item, you have to consume it first. This can be done by using the **mobileStoreConsumePurchase** *productID* command (Google Play Store only).

If you want to build apps for Amazon and/or Samsung Store, you have to use the newest LiveCode API.  

**How to use the new API?**

**Setup**

Before you can use IAP, you must set up products in each vendor's developer portal. In general terms, you have to:

- Create each product you want to sell, giving it a unique identifier. Note that for the Samsung Seller Office, the developer cannot choose the product identifier. This is assigned by the store.
- Submit the items for approval to the appropriate store. Some stores may require additional metadata, such as screenshots of your for sale items.
- Set up unique test accounts. The user is not charged when making a purchase using the test account details. This applies to Apple and Google. Amazon and Samsung have different methods for testing.

For more detailed store-specific information, you can have a look at the links below:

[Apple AppStore](https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnectInAppPurchase_Guide/Chapters/Introduction.html#//apple_ref/doc/uid/TP40013727-CH1-SW1)

[Google Play Store](http://developer.android.com/google/play/billing/billing_admin.html)

[Amazon Appstore](https://developer.amazon.com/public/apis/earn/in-app-purchasing/docs/submitting)

[Samsung Apps Store](http://developer.samsung.com/in-app-purchase) and more specifically click [here](http://img-developer.samsung.com/contents/cmm/EN_Samsung_IAP2.0_Programming_Guide_Overview_v1.0.pdf)

**Purchase Types**

  There are three classes of products users can purchase:
  
1.  One-time purchases that get "consumed". Typically, these items are called *consumables*. The user can buy as many times as they want (virtual coins/bullets in a game), except in apps built for the Google Play Store, where the user has to consume the purchased item first, and then buy (one) more.	
2.	One-time purchases that last forever, such as unlocking extra features, downloading new content once. These items are usually called *non-consumables*.
3.	Subscriptions where the app user pays a periodical fee to receive some ongoing service. Subscriptions can either be auto-renewable or non-renewable. 

Each vendor uses different terminology for these purchases : 

|        | Apple           | Google  |Amazon       |Samsung  |
----------------------------------------------------------------
|   one-time, gets consumed    | consumable | unmanaged |consumable       |consumable  |
| one-time, lasts forever     | non-consumable      |   managed |entitlement       |non-consumable  |
| subscriptions | auto-renewable , non-renewable  |    auto-renewable |auto-renewable       |non-renewable  |


**Testing**

Again, each store uses a different method of testing.

For the Apple AppStore, you can create test accounts. More details [here](https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnectInAppPurchase_Guide/Chapters/TestingInAppPurchases.html#//apple_ref/doc/uid/TP40013727-CH4-SW1).

For the Google Play Store, you can create test accounts as well as test using static responses. More details [here](http://developer.android.com/google/play/billing/billing_testing.html). Note that you cannot test subscriptions using the test account. This means that the test user will be charged when purchasing a subscription item. A possible workaround to this, is to log into the Google Wallet Service as a seller, using your Google Developer account details, and "refund" and then "cancel" the order of the subscription item that the test user had just purchased.

For the Amazon Appstore, you can test your app using SDK Tester. This is a developer tool that allows users of the Amazon Mobile App SDK to test their implementation in a production-like environment before submitting it to Amazon for publication. More details [here](https://developer.amazon.com/public/apis/earn/in-app-purchasing/docs/testing-iap).

For the Samsung Apps Store, Samsung IAP API offers three modes to test the service under various conditions : *Production Mode, Test Mode Success, Test Mode Fail*. During development period, you can select the mode in the Standalone Application Settings window. Before releasing your application, you must change to Production Mode. If you release your application in Test Mode, actual payments will not occur. More details on page 6 and 7 [here](http://img-developer.samsung.com/contents/cmm/EN_Samsung_IAP2.0_Android_Programming_Guide_v1.0.pdf).

Note that in Production Mode, your app can only interact with item groups with *sales* status. This information exists in the Samsung Seller Office. However, item groups are only given sales status after the app has been certified. In other words, you can test your app in Production Mode only after it has been certified by Samsung.

**Syntax**

Implementing in-app purchasing requires two way communication between your LiveCode app and the vendor's store. Here is the basic process:

- Your app sends a request to purchase a specific in-app purchase to the store
- The store verifies this and attempts to take payment
- If payment is successful the store notifies your app
- Your app unlocks features or downloads new content / fulfils the in-app purchase
- Your app tells the store that all actions associated with the purchase have been completed
- Store logs that in-app purchase has been completed

**Commands, Functions and Messages**

To determine if in-app purchasing is available use:

**mobileStoreCanMakePurchase()**

Returns *true* if in-app purchases can be made, *false* if not.

Throughout the purchase process, the store sends **purchaseStateUpdate** messages to your app which report any changes in the status of active purchases. The receipt of these messages can be switched on and off using:

**mobileStoreEnablePurchaseUpdates**  
**mobileStoreDisablePurchaseUpdates**

If you want to get information on a specific item (such as product identifier, product type, price etc), you can use:

**mobileStoreRequestProductDetails** *productID*

The *productID* is the identifier of the item you are interested. Then, the store sends a *productDetailsReceived* message, in case the request is successful, otherwise it sends a *productRequestError* message:

**productDetailsReceived** *productID*, *details*

The *productID* is the identifier of the item, and *details* is an array with the following keys - that are different depending on the store:

For Android stores (Google, Amazon, Samsung), the keys are:

- *productID* – identifier of the requested product
- *price* – price of the requested product
- *description* –  description of the requested product
- *title* – title of the requested product
- *itemType* – type of the requested product
- *itemImageUrl* – URL where the image (if any) of the requested product is stored
- *itemDownloadUrl* – URL to download the requested product
- *subscriptionDurationUnit* — subscription duration unit of the requested product
- *subscriptionDurationMultiplier* — subscription duration multiplier of the requested product

Note that some Android stores do not provide values for all the above keys. In this case, the value for the corresponding key will be empty.

For iTunes Connect store (Apple), the keys of *details* array are the following:

- *price* – price of the requested product
- *description* –  description of the requested product
- *title* – title of the requested product
- *currency code* – price currency code of the requested product
- *currency symbol* — currency symbol of the requested product
- *unicode description* — unicode description of the requested product
- *unicode title* — unicode title of the requested product
- *unicode currency symbol* — unicode currency symbol of the requested product

If **mobileStoreRequestProductDetails** is not successful, then a *productRequestError* message is sent :

**productRequestError** *productID*, *error*

The *productID* is the identifier of the item, and *error* is a string that describes the error.

Before sending a purchase request for a particular item, you have to specify the type of this item. To do this, use :

**mobileStoreSetProductType** *itemType*

The *itemType* can either be *subs* or *inapp*.

To create and send a request for a new purchase use:

**mobileStoreMakePurchase** *productID*, *quantity*, *developerPayload*

The *productID* is the identifier of the in-app purchase you created in the vendor's developer portal and wish to purchase. The *quantity* specifies the quantity of the in-app purchase to buy (iOS only - always "1" in Android) . The *developerPayload* is a string of less than 256 characters that will be returned with the purchase details once complete. Can be used to later identify a purchase response to a specific request (Android only).


To get a list of all known completed purchases use:

**mobileStorePurchasedProducts()**

It returns a return-separated list of product identifiers, of restored or newly bought purchases which are confirmed as complete. Note that in iOS, consumable products as well as non-renewable subscriptions will not be contained in this list.

Once a purchase is complete, you can retrieve the properties of the purchased product, using:

**mobileStoreProductProperty** *(productID, property)*

The parameters are as follows:

- *productID* – identifier of the requested product
- *property* – name of the purchase request property to get

Properties which can be queried can differ depending on the store: 

For the Samsung Apps Store (Android), you can query the properties:

- *title*– title of the purchased product
- *productId* – identifier of the purchased product
- *price* – price of the purchased product
- *currencyUnit* – currency unit of the product price
- *description* – description of the product as specified in the Samsung Seller Office
- *itemImageUrl* — URL where the image of the purchased product is stored
- *itemDownLoadUrl* — URL to download the purchased product 
- *paymentId* — payment identifier of the purchased product
- *purchaseId* — purchase identifier of the purchased product
- *purchaseDate* — purchase date, in milliseconds
- *verifyUrl* — IAP server URL for checking if the purchase is valid for the IAP server, using the *purchaseId* value

For the Google Play Store (Android), you can query the properties:

- *productId* – identifier of the purchased product
- *packageName* – application package from which the purchase originated
- *orderId* – unique order identifier for the transaction. This corresponds to the Google Wallet Order ID
- *purchaseTime* – time the product was purchased, in milliseconds
- *developerPayload*– developer-specified string that contains supplemental information about an order. You can specify a value for this in **mobileStoreMakePurchase**
- *purchaseToken*– token that uniquely identifies a purchase for a given item and user pair.
- *itemType* — type of the purchased item, *inapp* or *subs*
- *signature* — string containing the signature of the purchase data that was signed with the private key of the developer. The data signature uses the RSASSA-PKCS1-v1_5 scheme

For the Amazon Appstore (Android), you can query the properties:

- *productId* – identifier of the purchased product
- *itemType* — type of the purchased product. This can be *CONSUMABLE*, *ENTITLED* or *SUBSCRIPTION*
- *subscriptionPeriod* – string indicating the start and end date for subscription (for subscription products only)
- *purchaseToken*– purchase token that can be used from an external server to validate purchase

For Apple AppStore (iOS), you can query the properties:

- *quantity* – amount of item purchased. You can specify a value for this in **mobileStoreMakePurchase**
- *productId* – identifier of the purchased product
- *receipt* – block of data that can be used to confirm the purchase from a remote server with the iTunes Connect store 
- *purchaseDate* – date the purchase / restoration request was sent
- *transactionIdentifier* – unique identifier for a successful purchase / restoration request
- *originalPurchaseDate* –  date of the original purchase, for restored purchases 
- *originalTransactionIdentifier* – the transaction identifier of the original purchase, for restored purchases
- *originalReceipt* – the receipt for the original purchase, for restored purchases

Once you have sent your purchase request and it has been confirmed, you can then unlock or download new content to fulfil the requirements of the in-app purchase. You must inform the store once you have completely fulfiled the purchase using:

**mobileStoreConfirmPurchase** *productID*

Here, *productID* is the identifier of the product requested for purchase.

**mobileStoreConfirmPurchase** should only be called on a purchase request in the *paymentReceived* or *restored* state (more on the states of the purchase later). If you don't send this confirmation before the app is closed, **purchaseStateUpdate** messages for the purchase will be sent to your app the next time updates are enabled by calling the **mobileStoreEnablePurchaseUpdates** command.

To consume a purchased product use:

**mobileStoreConsumePurchase** *productID*

Here, *productID* is the identifier of the product requested for consumption. Note that this command is actually only used when interacting with the Google Play Store API. This is because the Google Play Store API has a restriction that ensures a consumable product is consumed before another instance is purchased. *Consume* means that the purchase is removed from the user's inventory of purchased items, allowing the user buy that product again.

Note that **mobileStoreConsumePurchase** must only be called on consumable products. If you call **mobileStoreConsumePurchase** on a non-consumable product, then you no longer own this product.

To instruct the store to re-send notifications of previously completed purchases use:

**mobileStoreRestorePurchases**

This would typically be called the first time an app is run after installation on a new device to restore any items bought through the app.

To get more detailed information about errors in the purchase request use:

**mobileStorePurchaseError** *(purchaseID)*


The store sends **purchaseStateUpdate** messages to notifies your app of any changes in state to the purchase request. These messages continue until you notify the store that the purchase is complete or it is cancelled.

**purchaseStateUpdate** *purchaseID, productID, state*

The state can be any one of the following:

- *sendingRequest* – the purchase request is being sent to the store / marketplace
- *paymentReceived* – the requested item has been paid for. The item should now be delivered to the user and confirmed via the mobileStoreConfirmPurchase command
- *alreadyEntitled* – the requested item is already owned, and cannot be purchased again
- *invalidSKU* – the requested item does not exist in the store listing
- *complete* – the purchase has now been paid for and delivered
- *restored* – the purchase has been restored after a call to mobileStoreRestorePurchases. The purchase should now be delivered to the user and confirmed via the mobileStoreConfirmPurchase command
- *cancelled* – the purchase was cancelled by the user before payment was received
- *error* – An error occurred during the payment request. More detailed information is available from the mobileStorePurchaseError function