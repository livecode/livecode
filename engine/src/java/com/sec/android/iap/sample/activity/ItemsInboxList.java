package com.sec.android.iap.sample.activity;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.sec.android.iap.sample.R;
import com.sec.android.iap.sample.adapter.ItemInboxListAdapter;
import com.sec.android.iap.sample.helper.SamsungIapHelper;
import com.sec.android.iap.sample.helper.SamsungIapHelper.OnGetInboxListListener;
import com.sec.android.iap.sample.helper.SamsungIapHelper.OnInitIapListener;
import com.sec.android.iap.sample.vo.InBoxVO;

public class ItemsInboxList extends Activity
    implements OnInitIapListener, OnGetInboxListListener
{
    private static final String  TAG = ItemsInboxList.class.getSimpleName();

    private ListView              mItemInboxListView    = null;
    private TextView              mNoDataTextView       = null;

    // Item Group ID of 3rd Party Application
    // ========================================================================
    private String                mItemGroupId          = null;
    // ========================================================================
    
    // Communication Helper between IAPService and 3rd Party Application
    // ========================================================================
    private SamsungIapHelper      mSamsungIapHelper     = null;
    // ========================================================================
    
    // For loading list of purchased item
    // ========================================================================
    /** ArrayList for list of purchased item */
    private ArrayList<InBoxVO>    mInboxVOList = new ArrayList<InBoxVO>();
    
    /** AdapterView for list of purchased item */
    private ItemInboxListAdapter  mItemInboxListAdapter = null;
    // ========================================================================
    
    private int mIapMode = SamsungIapHelper.IAP_MODE_COMMERCIAL;
    
    @Override
    protected void onCreate( Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );
        
        setContentView( R.layout.item_inbox_list_layout );

        // 1. intent 로 전달된 아이템 그룹 아이디와 모드를 저장한다.
        //    store Item Group Id and IapMode passed by Intent
        // ====================================================================
        Intent intent = getIntent();
        
        if( intent != null && intent.getExtras() != null 
                           && intent.getExtras().containsKey( "ItemGroupId" ) 
                           && intent.getExtras().containsKey( "IapMode" ) )
        {
            Bundle extras = intent.getExtras();

            mItemGroupId = extras.getString( "ItemGroupId" );
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
        mSamsungIapHelper = SamsungIapHelper.getInstance( this, mIapMode );
        
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
        
        // 4.OnGetInboxListListener 등록
        //   Register OnGetInboxListListener
        // ====================================================================
        mSamsungIapHelper.setOnGetInboxListListener( this );
        // ====================================================================
        
        // 5. IAP 패키지가 설치되어 있다면.
        //    If IAP Package is installed in your device
        // ====================================================================
        if( true == mSamsungIapHelper.isInstalledIapPackage( this ) )
        {
            // 1) 설치된 패키지가 유효한 패키지라면
            //    If IAP package installed in your device is valid
            // ================================================================
            if( true == mSamsungIapHelper.isValidIapPackage( this ) )
            {
                // show Progress Dialog
                // ------------------------------------------------------------
                mSamsungIapHelper.showProgressDialog( this );
                // ------------------------------------------------------------
                
                // 삼성 어카운트 계정 인증 진행
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
        // 6. If IAP Package is not installed in your device
        // ====================================================================
        else
        {
            mSamsungIapHelper.installIapPackage( this );
        }
        // ====================================================================
        
        // 7. 뷰 세팅
        //    set views
        // ====================================================================
        initView();
        // ====================================================================
    }
    
    
    /**
     * IAP Service 를 바이딩하고 정상적으로 바인딩 되었다면
     * initIAP() 메소드를 호출하여 IAP를 초기화 한다.
     * 
     * bind IAPService. If IAPService properly bound,
     * initIAP() method is called to initialize IAPService.
     */
    public void getItemInboxListService()
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
                    // initialize IAPService.
                    // safeGetItemInboxList method is called after IAPService
                    // is initialized
                    // --------------------------------------------------------
                    mSamsungIapHelper.safeInitIap( ItemsInboxList.this );
                    // --------------------------------------------------------
                }
                // ============================================================
                // 2) 서비스 바인드가 성공적으로 끝나지 않았을 경우
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
                             ItemsInboxList.this,
                             getString( R.string.in_app_purchase ), 
                             getString( R.string.msg_iap_service_bind_failed ),
                             false,
                             null );
                    // --------------------------------------------------------
                }
                // ============================================================
            }
        });
        // ====================================================================
    }
    
    
    /**
     * initialize views
     */
    public void initView()
    {
        mItemInboxListView = (ListView)findViewById( R.id.itemInboxList );
        mNoDataTextView    = (TextView)findViewById( R.id.noDataText );
        mNoDataTextView.setVisibility( View.GONE );
        
        mItemInboxListView.setEmptyView( mNoDataTextView );
        
        mItemInboxListAdapter = new ItemInboxListAdapter( this, 
                                                       R.layout.item_inbox_row, 
                                                       mInboxVOList );
        
        mItemInboxListView.setAdapter( mItemInboxListAdapter );
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
            // 삼성 어카운트 계정 인증 결과 처리
            // treat result of SamsungAccount authentication
            // ================================================================
            case SamsungIapHelper.REQUEST_CODE_IS_ACCOUNT_CERTIFICATION :
            {
                // 1) 삼성 계정 인증결과 성공인 경우
                //    If SamsungAccount authentication is succeed 
                // ------------------------------------------------------------
                if( RESULT_OK == _resultCode )
                {
                    // IAP 서비스를 통해 구매 아이템 목록을 얻는다.
                    // Get purchased item list via IAPService 
                    // --------------------------------------------------------
                    getItemInboxListService();
                    // --------------------------------------------------------
                }
                // ------------------------------------------------------------
                // 2) 삼성 계정 인증을 취소했을 경우
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
                           ItemsInboxList.this,
                           getString( R.string.dlg_title_samsungaccount_authentication ),
                           getString( R.string.msg_authentication_has_been_cancelled ),
                           false,
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
        Date d = new Date();
        SimpleDateFormat sdf = new SimpleDateFormat( "yyyyMMdd",
                                                     Locale.getDefault() );
        String today = sdf.format( d );
        
        mSamsungIapHelper.safeGetItemInboxTask( this, 
                                                mItemGroupId,
                                                1,
                                                15,
                                                "20130101",
                                                today );
    }
    
    @Override
    public void OnSucceedGetInboxList( ArrayList<InBoxVO>  _inboxList )
    {
        Log.i( TAG, "getInboxList has finished successfully" );
        
        mSamsungIapHelper.dismissProgressDialog();
        
        mInboxVOList.addAll( _inboxList );
        
        if( mInboxVOList.size() > 0 )
        {
            mItemInboxListView.setVisibility( View.VISIBLE );
            mNoDataTextView.setVisibility( View.GONE );
        }
        
        mItemInboxListAdapter.notifyDataSetChanged();
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