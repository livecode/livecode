/* Copyright (C) 2003-2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

package com.runrev.android;

import android.util.Log;
import android.media.*;
import android.app.*;
import android.content.*;
import android.view.KeyEvent;
import android.provider.ContactsContract;
import android.net.Uri;
import android.os.Build;
import android.database.Cursor;

import java.util.*;

public class Contact
{
	public static final String TAG = "revandroid.Contact";
	
	static class Pair<Tleft, Tright>
	{
		public final Tleft left;
		public final Tright right;
		
		Pair(Tleft p_left, Tright p_right)
		{
			left = p_left;
			right = p_right;
		}
	}
	
	static class ContactCategory
	{
		public final String name;
		public final String content_type;
		public final List<Pair<String, String>> properties;
		public final List<Pair<String, Integer>> label_map;
		public final String id_column;
		public final String label_column;
		
		ContactCategory(String p_name, String p_content_type, List<Pair<String, String>> p_properties, List<Pair<String, Integer>> p_label_map, String p_id_column, String p_label_column)
		{
			name = p_name;
			content_type = p_content_type;
			properties = p_properties;
			label_map = p_label_map;
			id_column = p_id_column;
			label_column = p_label_column;
		}
	}
	
	private static final ContactCategory s_property_categories[];
	private static final ContactCategory s_email_category;
	private static final ContactCategory s_phone_category;
	private static final ContactCategory s_address_category;
	
	static
	{
		// setup label maps
		ArrayList<Pair<String, Integer>> t_email_labels = new ArrayList<Pair<String, Integer>>();
		t_email_labels.add(new Pair<String, Integer>("home", ContactsContract.CommonDataKinds.Email.TYPE_HOME));
		t_email_labels.add(new Pair<String, Integer>("work", ContactsContract.CommonDataKinds.Email.TYPE_WORK));
		t_email_labels.add(new Pair<String, Integer>("other", ContactsContract.CommonDataKinds.Email.TYPE_OTHER));
		
		ArrayList<Pair<String, Integer>> t_phone_labels = new ArrayList<Pair<String, Integer>>();
		t_phone_labels.add(new Pair<String, Integer>("home", ContactsContract.CommonDataKinds.Phone.TYPE_HOME));
		t_phone_labels.add(new Pair<String, Integer>("work", ContactsContract.CommonDataKinds.Phone.TYPE_WORK));
		t_phone_labels.add(new Pair<String, Integer>("other", ContactsContract.CommonDataKinds.Phone.TYPE_OTHER));
		t_phone_labels.add(new Pair<String, Integer>("mobile", ContactsContract.CommonDataKinds.Phone.TYPE_MOBILE));
		t_phone_labels.add(new Pair<String, Integer>("main", ContactsContract.CommonDataKinds.Phone.TYPE_MAIN));
		t_phone_labels.add(new Pair<String, Integer>("homefax", ContactsContract.CommonDataKinds.Phone.TYPE_FAX_HOME));
		t_phone_labels.add(new Pair<String, Integer>("workfax", ContactsContract.CommonDataKinds.Phone.TYPE_FAX_WORK));
		t_phone_labels.add(new Pair<String, Integer>("otherfax", ContactsContract.CommonDataKinds.Phone.TYPE_OTHER_FAX));
		t_phone_labels.add(new Pair<String, Integer>("pager", ContactsContract.CommonDataKinds.Phone.TYPE_PAGER));

		ArrayList<Pair<String, Integer>> t_address_labels = new ArrayList<Pair<String, Integer>>();
		t_address_labels.add(new Pair<String, Integer>("home", ContactsContract.CommonDataKinds.StructuredPostal.TYPE_HOME));
		t_address_labels.add(new Pair<String, Integer>("work", ContactsContract.CommonDataKinds.StructuredPostal.TYPE_WORK));
		t_address_labels.add(new Pair<String, Integer>("other", ContactsContract.CommonDataKinds.StructuredPostal.TYPE_OTHER));
		
		// setup property maps
		List<Pair<String, String>> t_name_properties = new ArrayList<Pair<String, String>>();
		t_name_properties.add(new Pair<String, String>("firstname", ContactsContract.CommonDataKinds.StructuredName.GIVEN_NAME));
		t_name_properties.add(new Pair<String, String>("middlename", ContactsContract.CommonDataKinds.StructuredName.MIDDLE_NAME));
		t_name_properties.add(new Pair<String, String>("lastname", ContactsContract.CommonDataKinds.StructuredName.FAMILY_NAME));
		t_name_properties.add(new Pair<String, String>("prefix", ContactsContract.CommonDataKinds.StructuredName.PREFIX));
		t_name_properties.add(new Pair<String, String>("suffix", ContactsContract.CommonDataKinds.StructuredName.SUFFIX));
		t_name_properties.add(new Pair<String, String>("firstnamephonetic", ContactsContract.CommonDataKinds.StructuredName.PHONETIC_GIVEN_NAME));
		t_name_properties.add(new Pair<String, String>("middlenamephonetic", ContactsContract.CommonDataKinds.StructuredName.PHONETIC_MIDDLE_NAME));
		t_name_properties.add(new Pair<String, String>("lastnamephonetic", ContactsContract.CommonDataKinds.StructuredName.PHONETIC_FAMILY_NAME));

		List<Pair<String, String>> t_nickname_properties = new ArrayList<Pair<String, String>>();
		t_nickname_properties.add(new Pair<String, String>("nickname", ContactsContract.CommonDataKinds.Nickname.NAME));

		List<Pair<String, String>> t_organization_properties = new ArrayList<Pair<String, String>>();
		t_organization_properties.add(new Pair<String, String>("organization", ContactsContract.CommonDataKinds.Organization.COMPANY));
		t_organization_properties.add(new Pair<String, String>("jobtitle", ContactsContract.CommonDataKinds.Organization.TITLE));
		t_organization_properties.add(new Pair<String, String>("department", ContactsContract.CommonDataKinds.Organization.DEPARTMENT));

		List<Pair<String, String>> t_note_properties = new ArrayList<Pair<String, String>>();
		t_note_properties.add(new Pair<String, String>("note", ContactsContract.CommonDataKinds.Note.NOTE));
		
		List<Pair<String, String>> t_email_properties = new ArrayList<Pair<String, String>>();
		t_email_properties.add(new Pair<String, String>("email", ContactsContract.Data.DATA1));
		
		List<Pair<String, String>> t_phone_properties = new ArrayList<Pair<String, String>>();
		t_phone_properties.add(new Pair<String, String>("phone", ContactsContract.CommonDataKinds.Phone.NUMBER));
		
		List<Pair<String, String>> t_address_properties = new ArrayList<Pair<String, String>>();
		t_address_properties.add(new Pair<String, String>("street", ContactsContract.CommonDataKinds.StructuredPostal.STREET));
		t_address_properties.add(new Pair<String, String>("city", ContactsContract.CommonDataKinds.StructuredPostal.CITY));
		t_address_properties.add(new Pair<String, String>("state", ContactsContract.CommonDataKinds.StructuredPostal.REGION));
		t_address_properties.add(new Pair<String, String>("zip", ContactsContract.CommonDataKinds.StructuredPostal.POSTCODE));
		t_address_properties.add(new Pair<String, String>("country", ContactsContract.CommonDataKinds.StructuredPostal.COUNTRY));

		// setup categories
		ContactCategory t_name = new ContactCategory(null,
													 ContactsContract.CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE,
													 t_name_properties, null,
													 ContactsContract.CommonDataKinds.StructuredName.CONTACT_ID,
													 null);
		ContactCategory t_nickname = new ContactCategory(null,
														 ContactsContract.CommonDataKinds.Nickname.CONTENT_ITEM_TYPE,
														 t_nickname_properties, null,
														 ContactsContract.CommonDataKinds.Nickname.CONTACT_ID,
														 null);
		ContactCategory t_organization = new ContactCategory(null,
															 ContactsContract.CommonDataKinds.Organization.CONTENT_ITEM_TYPE,
															 t_organization_properties, null,
															 ContactsContract.CommonDataKinds.Organization.CONTACT_ID,
															 null);
		ContactCategory t_note = new ContactCategory(null,
													 ContactsContract.CommonDataKinds.Note.CONTENT_ITEM_TYPE,
													 t_note_properties, null,
													 ContactsContract.CommonDataKinds.Note.CONTACT_ID,
													 null);
		s_email_category = new ContactCategory("email",
											   ContactsContract.CommonDataKinds.Email.CONTENT_ITEM_TYPE,
											   t_email_properties, t_email_labels,
											   ContactsContract.CommonDataKinds.Email.CONTACT_ID,
											   ContactsContract.CommonDataKinds.Email.TYPE);
		s_phone_category = new ContactCategory("phone",
											   ContactsContract.CommonDataKinds.Phone.CONTENT_ITEM_TYPE,
											   t_phone_properties, t_phone_labels,
											   ContactsContract.CommonDataKinds.Phone.CONTACT_ID,
											   ContactsContract.CommonDataKinds.Phone.TYPE);
		s_address_category = new ContactCategory("address",
												 ContactsContract.CommonDataKinds.StructuredPostal.CONTENT_ITEM_TYPE,
												 t_address_properties, t_address_labels,
												 ContactsContract.CommonDataKinds.StructuredPostal.CONTACT_ID,
												 ContactsContract.CommonDataKinds.StructuredPostal.TYPE);
		
		s_property_categories = new ContactCategory[] {t_name, t_nickname, t_organization, t_note, s_email_category, s_phone_category, s_address_category};
	}
	
    private static final int PICK_CONTACT_RESULT = 6;
    private static final int CREATE_CONTACT_RESULT = 7;
    private static final int UPDATE_CONTACT_RESULT = 8;
    private static final int SHOW_CONTACT_RESULT = 9;
    
    protected Engine m_engine;
    protected Activity m_activity;
    
	public Contact(Engine p_engine, Activity p_activity)
	{
        m_engine = p_engine;
        m_activity = p_activity;
	}

    public void pickContact()
    {
        int t_contact_id = 0;
        Intent t_pick_intent = new Intent(Intent.ACTION_PICK, ContactsContract.Contacts.CONTENT_URI);
        m_activity.startActivityForResult(t_pick_intent, PICK_CONTACT_RESULT);
    }
    
    public void showContact(int p_contact_id)
    {
        int t_contact_id = 0;
        Intent t_view_intent = new Intent(Intent.ACTION_VIEW, ContentUris.withAppendedId(ContactsContract.Contacts.CONTENT_URI, p_contact_id));
        m_activity.startActivityForResult(t_view_intent, SHOW_CONTACT_RESULT);
    }
    
    public void createContact()
    {
        int t_contact_id = 0;
        Intent t_create_intent = new Intent(Intent.ACTION_INSERT, ContactsContract.Contacts.CONTENT_URI);
        m_activity.startActivityForResult(t_create_intent, CREATE_CONTACT_RESULT);
    }
    
	private ContentValues valuesForCategory(Map p_contact, ContactCategory p_category)
	{
		ContentValues t_values = new ContentValues();
		// loop over properties
		for (Pair<String, String> t_property : p_category.properties)
		{
			Object t_prop_obj = p_contact.get(t_property.left);
			if (t_prop_obj instanceof String)
			{
				t_values.put(t_property.right, (String)t_prop_obj);
			}
		}
		if (t_values.size() > 0)
		{
			t_values.put(ContactsContract.Data.MIMETYPE, p_category.content_type);
			return t_values;
		}
		return null;
	}
	
	private ArrayList<ContentValues> contactToContentValues(Map p_contact)
	{
		ArrayList<ContentValues> t_content_values = new ArrayList<ContentValues>();
		
		// loop over property categories
		for (ContactCategory t_category : s_property_categories)
		{
			// unlabeled properties
			if (t_category.label_map == null)
			{
				ContentValues t_values = valuesForCategory(p_contact, t_category);
				if (t_values != null)
					t_content_values.add(t_values);
			}
			else // labeled properties
			{
				Object t_cat_obj = p_contact.get(t_category.name);
				if (t_cat_obj instanceof Map)
				{
					Map t_cat_map = (Map)t_cat_obj;
					// loop over labels
					for (Pair<String, Integer> t_label : t_category.label_map)
					{
						Object t_label_obj = t_cat_map.get(t_label.left);
						if (t_label_obj instanceof Map)
						{
							Map t_label_map = (Map)t_label_obj;
							// loop over entries under label
							for (Object t_entry_obj : t_label_map.values())
							{
								ContentValues t_values = null;
								if (t_category.properties.size() == 1)
								{
									// for single-property labeled categories, entry should be a single string object
									if (t_entry_obj instanceof String)
									{
										t_values = new ContentValues();
										t_values.put(t_category.properties.get(0).right, (String)t_entry_obj);
										t_values.put(ContactsContract.Data.MIMETYPE, t_category.content_type);
									}
								}
								else
								{
									// for multi-property labeled categories, entry should be a map
									if (t_entry_obj instanceof Map)
										t_values = valuesForCategory((Map)t_entry_obj, t_category);
								}
								
								if (t_values != null)
								{
									t_values.put(t_category.label_column, t_label.right);
									t_content_values.add(t_values);
								}
							}
						}
					}
				}
			}
		}
		
		return t_content_values;
	}
	
	private void addNameToIntent(Map p_contact, Intent p_intent)
	{
		String t_full_name = "";
		Object t_name_obj;
		t_name_obj = p_contact.get("firstname");
		if (t_name_obj instanceof String)
			t_full_name += " " + (String)t_name_obj;
		t_name_obj = p_contact.get("middlename");
		if (t_name_obj instanceof String)
			t_full_name += " " + (String)t_name_obj;
		t_name_obj = p_contact.get("lastname");
		if (t_name_obj instanceof String)
			t_full_name += " " + (String)t_name_obj;
		
		if (t_full_name.length() > 0)
			p_intent.putExtra(ContactsContract.Intents.Insert.NAME, t_full_name);
		
		t_full_name = "";
		t_name_obj = p_contact.get("firstnamephonetic");
		if (t_name_obj instanceof String)
			t_full_name += " " + (String)t_name_obj;
		t_name_obj = p_contact.get("middlenamephonetic");
		if (t_name_obj instanceof String)
			t_full_name += " " + (String)t_name_obj;
		t_name_obj = p_contact.get("lastnamephonetic");
		if (t_name_obj instanceof String)
			t_full_name += " " + (String)t_name_obj;
		
		if (t_full_name.length() > 0)
			p_intent.putExtra(ContactsContract.Intents.Insert.PHONETIC_NAME, t_full_name);
	}
	
	private void addFirstAddressToIntent(Map p_contact, Intent p_intent)
	{
		Object t_address_obj = p_contact.get("address");
		if (!(t_address_obj instanceof Map))
			return;
		
		Map t_address_map = (Map)t_address_obj;
		for (Pair<String, Integer> t_label : s_address_category.label_map)
		{
			Object t_label_obj = t_address_map.get(t_label.left);
			if (t_label_obj instanceof Map)
			{
				Map t_label_map = (Map)t_label_obj;
				Object t_index_obj = t_label_map.get("1");
				if (t_index_obj instanceof Map)
				{
					addAddressToIntent((Map)t_index_obj, p_intent, t_label.right);
					return;
				}
			}
		}
	}
	
	private void addAddressToIntent(Map p_address, Intent p_intent, int p_type)
	{
		String t_full_address = "";
		Object t_address_obj;
		t_address_obj = p_address.get("street");
		if (t_address_obj instanceof String)
			t_full_address += (String)t_address_obj + "\n";

		String t_city_state_zip = "";
		t_address_obj = p_address.get("city");
		if (t_address_obj instanceof String)
			t_city_state_zip += (String)t_address_obj;
		t_address_obj = p_address.get("state");
		if (t_address_obj instanceof String)
		{
			if (t_city_state_zip.length() > 0)
				t_city_state_zip += ", ";
			t_city_state_zip += (String)t_address_obj;
		}
		t_address_obj = p_address.get("zip");
		if (t_address_obj instanceof String)
		{
			if (t_city_state_zip.length() > 0)
				t_city_state_zip += " ";
			t_city_state_zip += (String)t_address_obj;
		}
		
		if (t_city_state_zip.length() > 0)
			t_full_address += t_city_state_zip + "\n";
		
		t_address_obj = p_address.get("country");
		if (t_address_obj instanceof String)
			t_full_address += (String)t_address_obj + "\n";
		
		if (t_full_address.length() > 0)
		{
			p_intent.putExtra(ContactsContract.Intents.Insert.POSTAL, t_full_address);
			p_intent.putExtra(ContactsContract.Intents.Insert.POSTAL_TYPE, p_type);
			p_intent.putExtra(ContactsContract.Intents.Insert.POSTAL_ISPRIMARY, true);
		}
	}
	
	private void addCategoryToIntent(Map p_contact, ContactCategory p_category, Intent p_intent, String p_extras[], String p_extra_types[], String p_extra_isprimary)
	{
		Object t_cat_obj = p_contact.get(p_category.name);
		if (!(t_cat_obj instanceof Map))
			return;
		
		int t_limit = p_extras.length;
		int t_added_count = 0;
		
		Map t_cat_map = (Map)t_cat_obj;
		for (Pair<String, Integer> t_label : p_category.label_map)
		{
			Object t_label_obj = t_cat_map.get(t_label.left);
			if (t_label_obj instanceof Map)
			{
				Map t_label_map = (Map)t_label_obj;
				int t_index = 1;
				Object t_index_obj = t_label_map.get(String.valueOf(t_index));
				while (t_index_obj instanceof String)
				{
					p_intent.putExtra(p_extras[t_added_count], (String)t_index_obj);
					p_intent.putExtra(p_extra_types[t_added_count], (int)t_label.right);
					if (t_added_count == 0)
						p_intent.putExtra(p_extra_isprimary, true);

					t_added_count += 1;
					if (t_added_count == t_limit)
						return;
					
					t_index += 1;
					t_index_obj = t_label_map.get(String.valueOf(t_index));
				}
			}
		}
	}
	
	// set values using method supported prior to API version 11
	private void addPreHoneycombValuesToIntent(Map p_contact, Intent p_intent)
	{
		Object t_contact_obj;
		// set organization info
		t_contact_obj = p_contact.get("organization");
		if (t_contact_obj instanceof String)
			p_intent.putExtra(ContactsContract.Intents.Insert.COMPANY, (String)t_contact_obj);
		t_contact_obj = p_contact.get("jobtitle");
		if (t_contact_obj instanceof String)
			p_intent.putExtra(ContactsContract.Intents.Insert.JOB_TITLE, (String)t_contact_obj);
		
		// set notes
		t_contact_obj = p_contact.get("note");
		if (t_contact_obj instanceof String)
			p_intent.putExtra(ContactsContract.Intents.Insert.NOTES, (String)t_contact_obj);
		
		// set email addresses
		addCategoryToIntent(p_contact, s_email_category, p_intent,
							new String[] { ContactsContract.Intents.Insert.EMAIL,
								ContactsContract.Intents.Insert.SECONDARY_EMAIL,
								ContactsContract.Intents.Insert.TERTIARY_EMAIL },
							new String[] { ContactsContract.Intents.Insert.EMAIL_TYPE,
								ContactsContract.Intents.Insert.SECONDARY_EMAIL_TYPE,
								ContactsContract.Intents.Insert.TERTIARY_EMAIL_TYPE },
							ContactsContract.Intents.Insert.EMAIL_ISPRIMARY);
		
		// set phone numbers
		addCategoryToIntent(p_contact, s_phone_category, p_intent,
							new String[] { ContactsContract.Intents.Insert.PHONE,
								ContactsContract.Intents.Insert.SECONDARY_PHONE,
								ContactsContract.Intents.Insert.TERTIARY_PHONE },
							new String[] { ContactsContract.Intents.Insert.PHONE_TYPE,
								ContactsContract.Intents.Insert.SECONDARY_PHONE_TYPE,
								ContactsContract.Intents.Insert.TERTIARY_PHONE_TYPE },
							ContactsContract.Intents.Insert.PHONE_ISPRIMARY);
	}
	
	public void updateContact(Map p_contact, String p_title, String p_message, String p_alternate_name)
	{
		Intent t_update_intent = new Intent(Intent.ACTION_INSERT, ContactsContract.Contacts.CONTENT_URI);
		t_update_intent.setType(ContactsContract.RawContacts.CONTENT_TYPE);
		
		// add contact values ignored within "data" contact values
		addNameToIntent(p_contact, t_update_intent);
		addFirstAddressToIntent(p_contact, t_update_intent);
		
		// "data" row only supported from Honeycomb onward
		if (Build.VERSION.SDK_INT < 11)
			addPreHoneycombValuesToIntent(p_contact, t_update_intent);
		else
		{
			ArrayList<ContentValues> t_content;
			t_content = contactToContentValues(p_contact);

			if (t_content.size() > 0)
			{
				t_update_intent.putParcelableArrayListExtra("data", t_content);
			}
		}
		
        // Launch the intent
        m_activity.startActivityForResult(t_update_intent, CREATE_CONTACT_RESULT);
	}

	private void addCursorDataToContact(Map p_contact, Cursor p_cursor, List<Pair<String, String>> p_properties)
	{
		// loop over each property
		for (Pair<String, String> t_property : p_properties)
		{
			int t_column = p_cursor.getColumnIndex(t_property.right);
			if (t_column != -1)
			{
				String t_value = p_cursor.getString(t_column);
				if (t_value != null)
				{
					p_contact.put(t_property.left, t_value);
				}
			}
		}
	}
	
	public Map getContactData(int p_contact_id)
	{
		HashMap t_contact = new HashMap();
		// loop over categories
		for (ContactCategory t_category : s_property_categories)
		{
			if (t_category.label_map == null)
			{
				String t_query = ContactsContract.Data.MIMETYPE + " = '" + t_category.content_type + "' AND " + t_category.id_column + " = " + p_contact_id;
				Cursor t_cursor = m_activity.getContentResolver().query(ContactsContract.Data.CONTENT_URI, null, t_query, null, null);
				if (t_cursor != null && t_cursor.moveToFirst())
					addCursorDataToContact(t_contact, t_cursor, t_category.properties);
			}
			else
			{
				HashMap t_label_map = new HashMap();
				// loop over labels
				for (Pair<String, Integer> t_label : t_category.label_map)
				{
					String t_query = ContactsContract.Data.MIMETYPE + " = '" + t_category.content_type + "' AND " + t_category.id_column + " = " + p_contact_id + " and " + t_category.label_column + " = " + t_label.right;
					Cursor t_cursor = m_activity.getContentResolver().query(ContactsContract.Data.CONTENT_URI, null, t_query, null, null);
					if (t_cursor != null)
					{
						HashMap t_entry_map = new HashMap();
						
						int t_index = 1;
						// loop over each row
						while (t_cursor.moveToNext())
						{
							Object t_prop_obj = null;
							// if category has only one property
							if (t_category.properties.size() == 1)
							{
								int t_column = t_cursor.getColumnIndex(t_category.properties.get(0).right);
								if (t_column != -1)
									t_prop_obj = t_cursor.getString(t_column);
							}
							// else if category has more than one property
							else
							{
								// create map for category
								HashMap t_prop_map = new HashMap();
								addCursorDataToContact(t_prop_map, t_cursor, t_category.properties);
								if (t_prop_map.size() > 0)
									t_prop_obj = t_prop_map;
							}
							if (t_prop_obj != null)
							{
								t_entry_map.put(String.valueOf(t_index), t_prop_obj);
								t_index += 1;
							}
						}
						if (t_entry_map.size() > 0)
							t_label_map.put(t_label.left, t_entry_map);
					}
				}
				if (t_label_map.size() > 0)
					t_contact.put(t_category.name, t_label_map);
			}
		}
		
		if (t_contact.size() > 0)
			return t_contact;
		return null;
	}
    
    public void removeContact(int p_contact_id)
    {
        int t_contact_id = 0;
        Uri t_contact_to_delete = ContentUris.withAppendedId(ContactsContract.Contacts.CONTENT_URI, p_contact_id);
        if (t_contact_to_delete != null)
        {
            try
            {
                if (m_activity.getContentResolver().delete(t_contact_to_delete, null, null) > 0)
                {
                    t_contact_id = p_contact_id;
                }   
            }
            catch(Exception e)
            {
                Log.i("revandroid", "CT removeContact - Delete exception for: " + p_contact_id);
            }
        }
    }
    
	private ContentProviderOperation.Builder operationForCategory(Map p_contact, ContactCategory p_category)
	{
		ContentProviderOperation.Builder t_operation = ContentProviderOperation.newInsert(ContactsContract.Data.CONTENT_URI)
			.withValueBackReference(ContactsContract.Data.RAW_CONTACT_ID, 0)
			.withValue(ContactsContract.Data.MIMETYPE, p_category.content_type);

		// loop over properties
		for (Pair<String, String> t_property : p_category.properties)
		{
			Object t_prop_obj = p_contact.get(t_property.left);
			if (t_prop_obj instanceof String)
			{
				t_operation = t_operation.withValue(t_property.right, (String)t_prop_obj);
			}
		}
		return t_operation;
	}

	public int addContact(Map p_contact)
	{
		ArrayList<ContentProviderOperation> t_content_operations = new ArrayList<ContentProviderOperation>();
		
        t_content_operations.add(ContentProviderOperation.newInsert(ContactsContract.RawContacts.CONTENT_URI)
								 .withValue(ContactsContract.RawContacts.ACCOUNT_TYPE, null)
								 .withValue(ContactsContract.RawContacts.ACCOUNT_NAME, null)
								 .build());
		
		// loop over property categories
		for (ContactCategory t_category : s_property_categories)
		{
			// unlabeled properties
			if (t_category.label_map == null)
			{
				ContentProviderOperation.Builder t_operation;
				t_operation = operationForCategory(p_contact, t_category);
				if (t_operation != null)
					t_content_operations.add(t_operation.build());
			}
			else // labeled properties
			{
				Object t_cat_obj = p_contact.get(t_category.name);
				if (t_cat_obj instanceof Map)
				{
					Map t_cat_map = (Map)t_cat_obj;
					// loop over labels
					for (Pair<String, Integer> t_label : t_category.label_map)
					{
						Object t_label_obj = t_cat_map.get(t_label.left);
						if (t_label_obj instanceof Map)
						{
							Map t_label_map = (Map)t_label_obj;
							// loop over entries under label
							for (Object t_entry_obj : t_label_map.values())
							{
								ContentProviderOperation.Builder t_operation = null;
								if (t_category.properties.size() == 1)
								{
									// for single-property labeled categories, entry should be a single string object
									if (t_entry_obj instanceof String)
									{
										t_operation = ContentProviderOperation.newInsert(ContactsContract.Data.CONTENT_URI)
										.withValueBackReference(ContactsContract.Data.RAW_CONTACT_ID, 0)
										.withValue(ContactsContract.Data.MIMETYPE, t_category.content_type)
										.withValue(t_category.properties.get(0).right, (String)t_entry_obj);
									}
								}
								else
								{
									// for multi-property labeled categories, entry should be a map
									if (t_entry_obj instanceof Map)
										t_operation = operationForCategory((Map)t_entry_obj, t_category);
								}
								
								if (t_operation != null)
									t_content_operations.add(t_operation.withValue(t_category.label_column, (int)t_label.right).build());
							}
						}
					}
				}
			}
		}
		
		int t_contact_id = 0;
        try
        {
            ContentProviderResult[] t_insert_result = m_activity.getContentResolver().applyBatch(ContactsContract.AUTHORITY, t_content_operations);
            if (t_insert_result.length > 0)
            {
                Cursor t_database_cursor = m_activity.getContentResolver().query(ContactsContract.Contacts.CONTENT_URI, null, null, null, null);
                if (t_database_cursor.moveToLast() == true )
                {
                    t_contact_id = t_database_cursor.getInt(t_database_cursor.getColumnIndex("_id"));
                }
            }
        }
        catch (Exception e)
        {
            Log.i("revandroid", "CT addContact - Add exception: " + e.toString());
        }
        // Get the contact ID and return it here
		return t_contact_id;
	}
    
    public void findContact(String p_contact_name)
    {
        String t_contacts_found = null;
        Cursor t_database_cursor = m_activity.getContentResolver().query(ContactsContract.Contacts.CONTENT_URI, null, null, null, null);
        if (t_database_cursor.getCount() > 0)
        {
            while (t_database_cursor.moveToNext() == true)
            {
                String t_display_name = t_database_cursor.getString(t_database_cursor.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME));

                if (t_display_name.toLowerCase().indexOf(p_contact_name.toLowerCase()) >= 0)
                {
                    if (t_contacts_found == null)
                    {
                        t_contacts_found = "" + t_database_cursor.getLong(t_database_cursor.getColumnIndex("_id"));
                    }
                    else
                    {
                        t_contacts_found = t_contacts_found + "," + t_database_cursor.getLong(t_database_cursor.getColumnIndex("_id"));
                    }
                }
            }
        }
        // Get the contact IDs and return them here
        m_engine.doFindContact(t_contacts_found);
    }
}
