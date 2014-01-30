package com.sec.android.iap.sample.adapter;

import java.util.ArrayList;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import com.sec.android.iap.sample.R;
import com.sec.android.iap.sample.vo.ItemVO;

public class ItemListAdapter extends ArrayAdapter<ItemVO>
{
    private int               mResId    = 0;

    private LayoutInflater    mInflater = null;
    private ArrayList<ItemVO> mItems    = null;

    public ItemListAdapter
    (   
        Context             _context,
        int                 _resId,
        ArrayList<ItemVO>   _items
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
        TextView itemDescription;
    }
    

    @Override
    public View getView
    (   
        final int       _position,
        View            _convertView,
        final ViewGroup _parent
    )
    {
        final ItemVO vo = mItems.get( _position );
        ViewHolder vh;
        View v = _convertView;

        if( v == null )
        {
            vh = new ViewHolder();
            
            v = mInflater.inflate( mResId, null );
            
            vh.itemName        = (TextView)v.findViewById( R.id.itemName );
            
            vh.itemPriceString = (TextView)v.findViewById( 
                                                        R.id.itemPriceString );
            
            vh.itemType        = (TextView)v.findViewById( R.id.itemType );
            
            vh.itemDescription = (TextView)v.findViewById(
                                                        R.id.itemDescription );
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
        }
        else if( true == "01".equals( vo.getType() ) )
        {
            itemType += "NonConsumable";
        }
        else if( true == "02".equals( vo.getType() ) )
        {
            itemType += "Subscription ( " +
                        vo.getSubscriptionDurationMultiplier() + " " +
                        vo.getSubscriptionDurationUnit() + " )";
        }
        else
        {
            itemType += "Unsupported type";
        }
        
        vh.itemType.setText( itemType );
        
        vh.itemDescription.setText( vo.getItemDesc() );

        return v;
    }
}