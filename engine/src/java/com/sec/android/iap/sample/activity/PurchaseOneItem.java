package com.sec.android.iap.sample.activity;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import com.sec.android.iap.sample.R;
import com.sec.android.iap.sample.helper.SamsungIapHelper;
import com.sec.android.iap.sample.helper.SamsungIapHelper.OnInitIapListener;
import com.sec.android.iap.sample.vo.PurchaseVO;

public class PurchaseOneItem extends Activity implements OnInitIapListener
{
    private static final String  TAG = PurchaseOneItem.class.getSimpleName();

    // Item Group ID of 3rd Party Application
    // ========================================================================
    private String            mItemGroupId       = null;
    // ========================================================================
    
    // Purchase target item ID
    // ========================================================================
    private String            mItemId            = null;
    // ========================================================================
    
    // Communication Helper between IAPService and 3rd Party Application
    // ========================================================================
    private SamsungIapHelper  mSamsungIapHelper  = null;
    // ========================================================================
    
    private int mIapMode = SamsungIapHelper.IAP_MODE_COMMERCIAL;
    
    @Override
    protected void onCreate( Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );
        
        // 1. 아이템 그룹 아이디와 아이템 아이디를 저장한다.
        //    store Item Group Id, Item Id and IapMode passed by Intent
        // ====================================================================
        Intent intent = getIntent();
        
        if( intent != null && intent.getExtras() != null 
                           && intent.getExtras().containsKey( "ItemGroupId" ) 
                           && intent.getExtras().containsKey( "ItemId" )
                           && intent.getExtras().containsKey( "IapMode" ) )
        {
            Bundle extras = intent.getExtras();

            mItemGroupId = extras.getString( "ItemGroupId" );
            mItemId      = extras.getString( "ItemId" );
            mIapMode     = extras.getInt( "IapMode" );
        }
        else
        {
            Toast.makeText( this, 
                            R.string.invalid_activity_params,
                            Toast.LENGTH_LONG ).show();
            finish();
        }
        // ====================================================================
        
        // 2. SamsungIapHelper Instance 생성
        //    create SamsungIapHelper Instance
        //
        //    과금이 되지 않는 테스트 모드로 설정하고 싶다면, 
        //    SamsungIapHelper.IAP_MODE_TEST_SUCCESS 사용하세요.
        //    Billing does not want to set the test mode, 
        //                          SamsungIapHelper.IAP_MODE_TEST_SUCCESS use.
        // ====================================================================
        mSamsungIapHelper = SamsungIapHelper.getInstance( this , mIapMode );
        
        // 테스트를 위한 코드
        // For test...
        /*mSamsungIapHelper = new SamsungIapHelper( this ,
                                    SamsungIapHelper.IAP_MODE_TEST_SUCCESS );*/
        // ====================================================================
        
        // 3. OnInitIapListener 등록
        //    Register OnInitIapListener
        // ====================================================================
        mSamsungIapHelper.setOnInitIapListener( this );
        // ====================================================================
        
        // 4. IAP 패키지가 설치되어 있다면.
        //    If IAP Package is installed in your device
        // ====================================================================
        if( true == mSamsungIapHelper.isInstalledIapPackage( this ) )
        {
            // 1) 설치된 패키지가 유효한 패키지라면
            //    If IAP package installed in your device is valid
            // ================================================================
            if( true == mSamsungIapHelper.isValidIapPackage( this ) )
            {
                // show ProgressDialog
                // ------------------------------------------------------------
                mSamsungIapHelper.showProgressDialog( this );
                // ------------------------------------------------------------
                
                // 삼성어카운트 계정 인증 진행
                // process SamsungAccount authentication
                // ------------------------------------------------------------
                mSamsungIapHelper.startAccountActivity( this );
                // ------------------------------------------------------------
            }
            // ================================================================
            // 2) 설치된 패키지가 유효하지 않다면
            //    If IAP package installed in your device is not valid
            // ================================================================            
            else
            {
                // show alert dialog for invalid IAP Package
                // ------------------------------------------------------------
                mSamsungIapHelper.showIapDialog(
                                     this,
                                     getString( R.string.in_app_purchase ),           
                                     getString( R.string.invalid_iap_package ),
                                     true,
                                     null );
                // ------------------------------------------------------------
            }
            // ================================================================ 
        }
        // ====================================================================
        // 5. IAP 패키지가 설치되어 있지 않다면 
        //    If IAP Package is not installed in your device
        // ====================================================================
        else
        {
            mSamsungIapHelper.installIapPackage( this );
        }
        // ====================================================================
    }
    

    /**
     * IAP Service 를 바이딩하고 정상적으로 바인딩 되었다면
     * initIAP() 메소드를 호출하여 IAP를 초기화 한다.
     * 
     * bind IAPService. If IAPService properly bound,
     * initIAP() method is called to initialize IAPService.
     */
    public void bindIapService()
    {
        // 1. 서비스를 사용하기위해 Bind 처리를 한다.
        //    bind to IAPService
        // ====================================================================
        mSamsungIapHelper.bindIapService( 
                                       new SamsungIapHelper.OnIapBindListener()
        {
            @Override
            public void onBindIapFinished( int result )
            {
                // 1) 서비스 바인드가 성공적으로 끝났을 경우
                //    If successfully bound IAPService
                // ============================================================
                if ( result == SamsungIapHelper.IAP_RESPONSE_RESULT_OK )
                {
                    // IAPService를 초기화한다.
                    // 초기화 성공 후에 PurchaseMethodListActivity가 호출된다.
                    // initialize IAPService.
                    // PurchaseMethodListActivity is called after IAPService
                    // is initialized
                    // --------------------------------------------------------
                    mSamsungIapHelper.safeInitIap( PurchaseOneItem.this );
                    // --------------------------------------------------------
                }
                // ============================================================
                // 2) 서비스 바인드가 실패했을 경우
                //    If IAPService is not bound correctly
                // ============================================================
                else
                {
                    // dismiss ProgressDialog
                    // --------------------------------------------------------
                    mSamsungIapHelper.dismissProgressDialog();
                    // --------------------------------------------------------
                    
                    // show alert dialog for bind failure
                    // --------------------------------------------------------
                    mSamsungIapHelper.showIapDialog(
                             PurchaseOneItem.this,
                             getString( R.string.in_app_purchase ), 
                             getString( R.string.msg_iap_service_bind_failed ),
                             true,
                             null );
                    // --------------------------------------------------------
                }
                // ============================================================
            }
        });
        // ====================================================================
    }
    

    /**
     * Samsung Account 인증 결과와 IAP 결과 처리를 한다.
     * treat result of SamsungAccount Authentication and IAPService 
     */
    @Override
    protected void onActivityResult
    (   
        int     _requestCode,
        int     _resultCode,
        Intent  _intent
    )
    {
        switch( _requestCode )
        {
            // 1. IAP 결제 결과 처리
            //    treat result of IAPService
            // ================================================================
            case SamsungIapHelper.REQUEST_CODE_IS_IAP_PAYMENT:
            {
                if( null == _intent )
                {
                    break;
                }
                
                Bundle extras         = _intent.getExtras();
                
                String itemId         = "";
                String thirdPartyName = "";
                
                // payment success   : 0
                // payment cancelled : 1
                // ============================================================
                int statusCode        = 1;
                // ============================================================
                
                String errorString    = "";
                PurchaseVO purchaseVO = null;
                
                // 1) IAP 에서 전달된 Bundle 정보가 존재할 경우
                //    If there is bundle passed from IAP
                // ------------------------------------------------------------
                if( null != extras )
                {
                    thirdPartyName = extras.getString(
                                  SamsungIapHelper.KEY_NAME_THIRD_PARTY_NAME );
                    
                    statusCode = extras.getInt( 
                                       SamsungIapHelper.KEY_NAME_STATUS_CODE );
                    
                    errorString = extras.getString( 
                                      SamsungIapHelper.KEY_NAME_ERROR_STRING );
                    
                    itemId = extras.getString(
                                           SamsungIapHelper.KEY_NAME_ITEM_ID );
                    
                    // 로그 출력 : 릴리즈 전에 삭제하세요.
                    // print log : Please remove before release
                    // --------------------------------------------------------
                    Log.i( TAG, "3rdParty Name : " + thirdPartyName + "\n" +
                                "ItemId        : " + itemId + "\n" +
                                "StatusCode    : " + statusCode + "\n" +
                                "errorString   : " + errorString );
                    // --------------------------------------------------------
                }
                // ------------------------------------------------------------
                // 2) IAP 에서 전달된 Bundle 정보가 존재하지 않는 경우
                //    If there is no bundle passed from IAP
                // ------------------------------------------------------------
                else
                {
                    mSamsungIapHelper.showIapDialog(
                        this,  
                        getString( R.string.dlg_title_payment_error ), 
                        getString( R.string.msg_payment_was_not_processed_successfully ),
                        true,
                        null );
                }
                // ------------------------------------------------------------
                
                // 3) 결제가 성공했을 경우
                //    If payment was not cancelled
                // ------------------------------------------------------------
                if( RESULT_OK == _resultCode )
                {
                    // a. IAP 에서 넘어온 결제 결과가 성공인 경우 verifyurl 과 
                    //    purchaseId 값으로 서버에 해당 결제가 유효한 지 확인한다.
                    //    if Payment succeed
                    // --------------------------------------------------------
                    if( statusCode == SamsungIapHelper.IAP_ERROR_NONE )
                    {
                        // 정상적으로 결제가 되었으므로 PurchaseVO를 생성한다.
                        // make PurcahseVO
                        // ----------------------------------------------------
                        purchaseVO = new PurchaseVO( extras.getString(
                                   SamsungIapHelper.KEY_NAME_RESULT_OBJECT ) );
                        // ----------------------------------------------------
                        
                        // 결제 유효성을 확인한다.
                        // verify payment result
                        // ----------------------------------------------------
                        mSamsungIapHelper.verifyPurchaseResult( 
                                                          PurchaseOneItem.this,
                                                          purchaseVO );
                        // ----------------------------------------------------
                    }
                    // --------------------------------------------------------
                    // b. IAP 에서 넘어온 결제 결과가 실패인 경우 에러메시지를 출력
                    //    Payment failed 
                    // --------------------------------------------------------
                    else
                    {
                        mSamsungIapHelper.showIapDialog( 
                                 this,
                                 getString( R.string.dlg_title_payment_error ),           
                                 errorString,
                                 true,
                                 null );
                    }
                    // --------------------------------------------------------
                }
                // ------------------------------------------------------------
                // 4) 결제를 취소했을 경우
                //    If payment was cancelled
                // ------------------------------------------------------------
                else if( RESULT_CANCELED == _resultCode )
                {
                    mSamsungIapHelper.showIapDialog(
                             this,
                             getString( R.string.dlg_title_payment_cancelled ),
                             getString( R.string.dlg_msg_payment_cancelled ),
                             true,
                             null );
                }
                // ------------------------------------------------------------
                
                break;
            }
            // ================================================================
            
            // 2. 삼성 어카운트 계정 인증 결과 처리
            //    treat result of SamsungAccount authentication
            // ================================================================
            case SamsungIapHelper.REQUEST_CODE_IS_ACCOUNT_CERTIFICATION :
            {
                // 2) 삼성 계정 인증결과 성공인 경우
                //    If SamsungAccount authentication is succeed 
                // ------------------------------------------------------------
                if( RESULT_OK == _resultCode )
                {
                    // IAP Service 바인드 및 초기화 진행
                    // start binding and initialization for IAPService 
                    // --------------------------------------------------------
                    bindIapService();
                    // --------------------------------------------------------
                }
                // ------------------------------------------------------------
                // 3) 삼성 계정 인증을 취소했을 경우
                //    If SamsungAccount authentication is cancelled
                // ------------------------------------------------------------
                else if( RESULT_CANCELED == _resultCode )
                {
                    // 프로그래스를 dismiss 처리합니다.
                    // dismiss ProgressDialog for SamsungAccount Authentication
                    // --------------------------------------------------------
                    mSamsungIapHelper.dismissProgressDialog();
                    // --------------------------------------------------------
                    
                    mSamsungIapHelper.showIapDialog(
                        this,
                        getString( R.string.dlg_title_samsungaccount_authentication ),
                        getString( R.string.msg_authentication_has_been_cancelled ),
                        true,
                        null );
                }
                // ------------------------------------------------------------
                
                break;
            }
            // ================================================================
        }
    }

    @Override
    public void onSucceedInitIap()
    {
        // 프로그래스 다이얼로그를 dismiss 한다
        // dismiss ProgressDialog
        // ====================================================================
        mSamsungIapHelper.dismissProgressDialog();
        // ====================================================================
        
        // 초기화가 정상적으로 완료되었기 때문에 IAP 결제방법 목록을 호출한다.
        // call PurchaseMethodListActivity of IAP
        // ====================================================================
        mSamsungIapHelper.startPurchase( 
                                  PurchaseOneItem.this, 
                                  SamsungIapHelper.REQUEST_CODE_IS_IAP_PAYMENT, 
                                  mItemGroupId,
                                  mItemId );
        // ====================================================================
    }
    
    @Override
    protected void onDestroy()
    {
        super.onDestroy();

        if( null != mSamsungIapHelper )
        {
            mSamsungIapHelper.stopRunningTask();
        }
    }
}