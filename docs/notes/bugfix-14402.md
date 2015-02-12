#    [[ In App Purchase ]] Calling mobileStoreRestorePurchases when there are no previous purchases to restore

**What has changed**

Previously, if mobileStoreRestorePurchases was called and no previous purchases were made with that user account, nothing happened. 
Now, a purchaseStateUpdate message is sent with state=restored and productID=""
