package com.sec.android.iap.sample.adapter;

import java.util.ArrayList;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import com.sec.android.iap.sample.R;
import com.sec.android.iap.sample.vo.InBoxVO;

public class ItemInboxListAdapter extends ArrayAdapter<InBoxVO>
{
    private int                     mResId    = 0;

    private LayoutInflater          mInflater = null;
    private ArrayList<InBoxVO>      mItems    = null;

    
    public ItemInboxListAdapter
    (   
        Context                 _context,
        int                     _resId,
        ArrayList<InBoxVO>      _items
    )
    {
        super( _context, _resId, _items );

        mResId    = _resId;
        mItems    = _items;
        mInflater = (LayoutInflater)_context.getSystemService( 
                                             Context.LAYOUT_INFLATER_SERVICE );
    }

    
    public static class ViewHolder
    {
        TextView itemName;
        TextView itemPriceString;
        TextView itemType;
        TextView paymentId;
        TextView purchaseDate;
        TextView expireDate;
    }

    
    @Override
    public View getView
    (   
        final int         _position,
        View              _convertView,
        final ViewGroup   _parent
    )
    {
        final InBoxVO vo = mItems.get( _position );
        ViewHolder vh;
        View v = _convertView;

        if( v == null )
        {
            vh = new ViewHolder();
            v = mInflater.inflate( mResId, null );
            
            vh.itemName        = (TextView)v.findViewById( R.id.itemName );
            vh.itemPriceString = (TextView)v.findViewById( R.id.itemPriceString );
            vh.itemType        = (TextView)v.findViewById( R.id.itemType );
            vh.paymentId       = (TextView)v.findViewById( R.id.paymentId );
            vh.purchaseDate    = (TextView)v.findViewById( R.id.purchaseDate );
            vh.expireDate      = (TextView)v.findViewById( R.id.expireDate );
            
            v.setTag( vh );
        }
        else
        {
            vh = (ViewHolder)v.getTag();
        }

        vh.itemName.setText( vo.getItemName() );
        vh.itemPriceString.setText( vo.getItemPriceString() );
        
        String itemType = "Type : ";
        
        if( true == "00".equals( vo.getType() ) )
        {
            itemType += "Consumable";
            vh.expireDate.setVisibility( View.GONE );
        }
        else if( true == "01".equals( vo.getType() ) )
        {
            itemType += "NonConsumable";
            vh.expireDate.setVisibility( View.GONE );
        }
        else if( true == "02".equals( vo.getType() ) )
        {
            itemType += "Subscription";
            vh.expireDate.setVisibility( View.VISIBLE );
        }
        else
        {
            itemType += "Unsupported type";
            vh.expireDate.setVisibility( View.GONE );
        }
        vh.itemType.setText( itemType );
        
        vh.paymentId.setText( "Payment ID : " + vo.getPaymentId() );
        vh.purchaseDate.setText( "Purchase Date : " + vo.getPurchaseDate() );
        vh.expireDate.setText( "Expire Date : " + vo.getSubscriptionEndDate() );
        
        return v;
    }
}
