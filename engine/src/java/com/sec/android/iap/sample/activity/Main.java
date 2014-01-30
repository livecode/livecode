package com.sec.android.iap.sample.activity;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Spinner;

import com.sec.android.iap.sample.R;
import com.sec.android.iap.sample.helper.SamsungIapHelper;

public class Main extends Activity
{
    private Spinner          mItemTypeSpinner  = null;
    private SamsungIapHelper mSamsungIapHelper = null;
    
    // ※ 주의
    // SamsungIapHelper.IAP_MODE_TEST_SUCCESS 모드는 실제 과금이 발생하지 않는
    // 테스트 모드입니다. 릴리즈할 때는 반드시 SamsungIapHelper.IAP_MODE_COMMERCIAL
    // 모드로 설정해야 합니다.SamsungIapHelper.IAP_MODE_COMMERCIAL 모드에서만
    // 실제 결제가 발생합니다.
    //
    // ※ CAUTION
    // SamsungIapHelper.IAP_MODE_TEST_SUCCESS mode is test mode that does not
    // occur actual billing.
    // When you release the SamsungIapHelper.IAP_MODE_COMMERCIAL mode
    // must be set.
    // In SamsungIapHelper.IAP_MODE_COMMERCIAL mode only
    // actual billing is occurred.
    // ========================================================================
    private static final int IAP_MODE = SamsungIapHelper.IAP_MODE_TEST_SUCCESS;
    // ========================================================================
    
    // Item Group ID of 3rd Party Application
    // ========================================================================
    private static final String ITEM_GROUP_ID    = "100000100010";
    // ========================================================================
    
    // Item ID for test button of purchase one item
    // ========================================================================
    private static final String ITEM_ID          = "000001000018";
    // ========================================================================
    
    @Override
    protected void onCreate( Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );
        setContentView( R.layout.main_layout );
        
        mItemTypeSpinner = (Spinner)findViewById( R.id.spinner_item_type );
    }
    
    public void onClick( View v )
    {
        if ( null == v )
        {
            return;
        }
        
        switch ( v.getId() )
        {
            // Call ItemList
            // ================================================================
            case R.id.btn_get_item_list :
            {
                String itemType      = null;
                int    itemTypeIndex = mItemTypeSpinner.getSelectedItemPosition();
                
                switch( itemTypeIndex )
                {
                    case 0 :
                    {
                        itemType = SamsungIapHelper.ITEM_TYPE_CONSUMABLE;
                        break;
                    }
                    case 1 :
                    {
                        itemType = SamsungIapHelper.ITEM_TYPE_NON_CONSUMABLE;
                        break;
                    }
                    case 2 :
                    {
                        itemType = SamsungIapHelper.ITEM_TYPE_SUBSCRIPTION;
                        break;
                    }
                    case 3 :
                    {
                        itemType = SamsungIapHelper.ITEM_TYPE_ALL;
                        break;
                    }
                }
                
                Intent intent = new Intent( Main.this, ItemList.class );
                intent.putExtra( "ItemType", itemType );
                intent.putExtra( "ItemGroupId", ITEM_GROUP_ID );
                intent.putExtra( "IapMode", IAP_MODE );
                
                startActivity( intent );

                break;
            }
            // ================================================================
            // Call ItemInboxList
            // ================================================================
            case R.id.btn_get_inbox_list :
            {
                Intent intent = new Intent( Main.this, ItemsInboxList.class );
                intent.putExtra( "ItemGroupId", ITEM_GROUP_ID );
                intent.putExtra( "IapMode", IAP_MODE );                                        
                
                startActivity( intent );
                
                break;
            }
            // ================================================================
            // Call PaymentMethodListActivity
            // ================================================================
            case R.id.btn_purchase_one_item :
            {
                Intent intent = new Intent( Main.this, PurchaseOneItem.class );
                intent.putExtra( "ItemGroupId", ITEM_GROUP_ID );
                intent.putExtra( "ItemId", ITEM_ID );
                intent.putExtra( "IapMode", IAP_MODE );
                
                startActivity( intent );
                break;
            }
            // ================================================================
        }
    }
    
    @Override
    protected void onDestroy()
    {
        mSamsungIapHelper = SamsungIapHelper.getInstance( this, IAP_MODE );                                        
        
        if( null != mSamsungIapHelper )
        {
            mSamsungIapHelper.stopRunningTask();
            mSamsungIapHelper.dispose();
        }
        super.onDestroy();
    }
}