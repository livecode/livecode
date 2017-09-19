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

#include "prefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"


#include "handler.h"
#include "scriptpt.h"
#include "variable.h"
#include "statemnt.h"

#include "deploy.h"

////////////////////////////////////////////////////////////////////////////////

#if DEPLOY_DMG
// A Mac OS X device image has the following structure:
//   DeviceHeader
//   DevicePartion*
//   <partition data>
//

enum
{
	kDeviceHeaderSignature = 0x4552,
	kDevicePartitionSignature = 0x504D
};

struct DeviceHeader
{
	unsigned short    sbSig;         /* device signature */
	unsigned short    sbBlkSize;     /* block size of the device*/
	unsigned long     sbBlkCount;    /* number of blocks on the device*/
	unsigned short    sbDevType;     /* reserved */
	unsigned short    sbDevId;       /* reserved */
	unsigned long     sbData;        /* reserved */
	unsigned short    sbDrvrCount;   /* number of driver descriptor entries */
	unsigned long     ddBlock;       /* first driver's starting block */
	unsigned short    ddSize;        /* driver's size, in 512-byte blocks */
	unsigned short    ddType;        /* operating system type (MacOS = 1) */
	unsigned short    ddPad[243];    /* additional drivers, if any */
};

struct DevicePartition
{
   unsigned short    pmSig;         /* partition signature */
   unsigned short    pmSigPad;      /* reserved */
   unsigned long     pmMapBlkCnt;   /* number of blocks in partition map */
   unsigned long     pmPyPartStart; /* first physical block of partition */
   unsigned long     pmPartBlkCnt;  /* number of blocks in partition */
   unsigned char     pmPartName[32];/* partition name */
   unsigned char     pmParType[32]; /* partition type */
   unsigned long     pmLgDataStart; /* first logical block of data area */
   unsigned long     pmDataCnt;     /* number of blocks in data area */
   unsigned long     pmPartStatus;  /* partition status information */
   unsigned long     pmLgBootStart; /* first logical block of boot code */
   unsigned long     pmBootSize;    /* size of boot code, in bytes */
   unsigned long     pmBootAddr;    /* boot code load address */
   unsigned long     pmBootAddr2;   /* reserved */
   unsigned long     pmBootEntry;   /* boot code entry point */
   unsigned long     pmBootEntry2;  /* reserved */
   unsigned long     pmBootCksum;   /* boot code checksum */
   unsigned char     pmProcessor[16];  /* processor type */
   unsigned short    pmPad[188];    /* reserved */
};

// A HFS+ Volume has the following structure:
//   Reserved (1024 bytes)
//   Volume Header
//   ...
//   Allocation File
//   ...
//   Extents Overflow File
//   ...
//   Catalog File
//   ...
//   Attributes File
//   ...
//   Startup File
//   ...
//   Alternate Volume Header
//   Reserved (512 bytes)

// HFS+ Dates are stored as the number of seconds since '1904-01-01 00:00 +0000'.

struct HFSUniStr255
{
	uint16_t length;
	unichar_t unicode[255];
};

struct HFSPlusBSDInfo
{
    uint32_t ownerID;
    uint32_t groupID;
    uint8_t adminFlags;
    uint8_t ownerFlags;
    uint16_t fileMode;
    union {
        uint32_t iNodeNum;
        uint32_t linkCount;
        uint32_t rawDevice;
    } special;
};

struct HFSPlusExtentDescriptor
{
	uint32_t startBlock;
	uint32_t blockCount;
};

struct HFSPlusForkData
{
    uint64_t logicalSize;
    uint32_t clumpSize;
    uint32_t totalBlocks;
    HFSPlusExtentDescriptor extents[8];
};

typedef uint32_t HFSCatalogNodeID;

struct HFSPlusVolumeHeader
{
    uint16_t signature;
    uint16_t version;
    uint32_t attributes;
    uint32_t lastMountedVersion;
    uint32_t journalInfoBlock;
 
    uint32_t createDate;
    uint32_t modifyDate;
    uint32_t backupDate;
    uint32_t checkedDate;
 
    uint32_t fileCount;
    uint32_t folderCount;
 
    uint32_t blockSize;
    uint32_t totalBlocks;
    uint32_t freeBlocks;
 
    uint32_t nextAllocation;
    uint32_t rsrcClumpSize;
    uint32_t dataClumpSize;
    HFSCatalogNodeID nextCatalogID;
 
    uint32_t writeCount;
    uint64_t encodingsBitmap;
 
    uint32_t finderInfo[8];
 
	union
	{
		struct
		{
			HFSPlusForkData allocationFile;
			HFSPlusForkData extentsFile;
			HFSPlusForkData catalogFile;
			HFSPlusForkData attributesFile;
			HFSPlusForkData startupFile;
		};
		HFSPlusForkData specialFiles[5];
	};
};

#pragma pack(push)
#pragma pack(1)
struct HFSPlusCatalogKey
{
    uint16_t keyLength;
    HFSCatalogNodeID parentID;
    HFSUniStr255 nodeName;
};
#pragma pack(pop)

// Volume Attributes
// The attributes field of a volume header is treated as a set of one-bit flags.
// The definition of the bits is given by the constants listed below.
enum
{
    /* Bits 0-6 are reserved */
    kHFSVolumeHardwareLockBit       =  7,
    kHFSVolumeUnmountedBit          =  8,
    kHFSVolumeSparedBlocksBit       =  9,
    kHFSVolumeNoCacheRequiredBit    = 10,
    kHFSBootVolumeInconsistentBit   = 11,
    kHFSCatalogNodeIDsReusedBit     = 12,
    kHFSVolumeJournaledBit          = 13,
    /* Bit 14 is reserved */
    kHFSVolumeSoftwareLockBit       = 15
    /* Bits 16-31 are reserved */
};

enum {
    kHFSPlusFolderRecord        = 0x0001,
    kHFSPlusFileRecord          = 0x0002,
    kHFSPlusFolderThreadRecord  = 0x0003,
    kHFSPlusFileThreadRecord    = 0x0004
};

struct HFSFinderPoint {
  int16_t v;
  int16_t h;
};

struct HFSFinderRect {
  int16_t top;
  int16_t left;
  int16_t bottom;
  int16_t right;
};

/* OSType is a 32-bit value made by packing four 1-byte characters 
   together. */
typedef uint32_t HFSFinderFourCharCode;
typedef HFSFinderFourCharCode HFSFinderOSType;

/* Finder flags (finderFlags, fdFlags and frFlags) */
enum {
  kIsOnDesk       = 0x0001,     /* Files and folders (System 6) */
  kColor          = 0x000E,     /* Files and folders */
  kIsShared       = 0x0040,     /* Files only (Applications only) If */
                                /* clear, the application needs */
                                /* to write to its resource fork, */
                                /* and therefore cannot be shared */
                                /* on a server */
  kHasNoINITs     = 0x0080,     /* Files only (Extensions/Control */
                                /* Panels only) */
                                /* This file contains no INIT resource */
  kHasBeenInited  = 0x0100,     /* Files only.  Clear if the file */
                                /* contains desktop database resources */
                                /* ('BNDL', 'FREF', 'open', 'kind'...) */
                                /* that have not been added yet.  Set */
                                /* only by the Finder. */
                                /* Reserved for folders */
  kHasCustomIcon  = 0x0400,     /* Files and folders */
  kIsStationery   = 0x0800,     /* Files only */
  kNameLocked     = 0x1000,     /* Files and folders */
  kHasBundle      = 0x2000,     /* Files only */
  kIsInvisible    = 0x4000,     /* Files and folders */
  kIsAlias        = 0x8000      /* Files only */
};

/* Extended flags (extendedFinderFlags, fdXFlags and frXFlags) */
enum {
  kExtendedFlagsAreInvalid    = 0x8000, /* The other extended flags */
                                        /* should be ignored */
  kExtendedFlagHasCustomBadge = 0x0100, /* The file or folder has a */
                                        /* badge resource */
  kExtendedFlagHasRoutingInfo = 0x0004  /* The file contains routing */
                                        /* info resource */
};

struct HFSFinderFileInfo {
  HFSFinderOSType fileType;           /* The type of the file */
  HFSFinderOSType fileCreator;        /* The file's creator */
  uint16_t finderFlags;
  HFSFinderPoint location;           /* File's location in the folder. */
  uint16_t reservedField;
};

struct HFSFinderExtendedFileInfo {
  int16_t reserved1[4];
  uint16_t extendedFinderFlags;
  int16_t    reserved2;
  int32_t putAwayFolderID;
};

struct HFSFinderFolderInfo {
  HFSFinderRect windowBounds;       /* The position and dimension of the */
                                /* folder's window */
  uint16_t finderFlags;
  HFSFinderPoint location;           /* Folder's location in the parent */
                                /* folder. If set to {0, 0}, the Finder */
                                /* will place the item automatically */
  uint16_t reservedField;
};

struct HFSFinderExtendedFolderInfo {
  HFSFinderPoint scrollPosition;     /* Scroll position (for icon views) */
  int32_t reserved1;
  uint16_t extendedFinderFlags;
  int16_t reserved2;
  int32_t putAwayFolderID;
};

struct HFSPlusCatalogFolder {
    int16_t recordType;
    uint16_t flags;
    uint32_t valence;
    HFSCatalogNodeID folderID;
    uint32_t createDate;
    uint32_t contentModDate;
    uint32_t attributeModDate;
    uint32_t accessDate;
    uint32_t backupDate;
    HFSPlusBSDInfo permissions;
    HFSFinderFolderInfo userInfo;
    HFSFinderExtendedFolderInfo finderInfo;
    uint32_t textEncoding;
    uint32_t reserved;
};

struct HFSPlusCatalogFile {
    int16_t recordType;
    uint16_t flags;
    uint32_t reserved1;
    HFSCatalogNodeID fileID;
    uint32_t createDate;
    uint32_t contentModDate;
    uint32_t attributeModDate;
    uint32_t accessDate;
    uint32_t backupDate;
    HFSPlusBSDInfo permissions;
    HFSFinderFileInfo userInfo;
    HFSFinderExtendedFileInfo finderInfo;
    uint32_t textEncoding;
    uint32_t reserved2;
 
    HFSPlusForkData dataFork;
    HFSPlusForkData resourceFork;
};

struct HFSPlusCatalogThread
{
    int16_t              recordType;
    int16_t              reserved;
    HFSCatalogNodeID    parentID;
    HFSUniStr255        nodeName;
};

//////////

#pragma pack(push)
#pragma pack(1)

struct BTNodeDescriptor
{
    uint32_t fLink;
    uint32_t bLink;
    int8_t kind;
    uint8_t height;
    uint16_t numRecords;
    uint16_t reserved;
};

enum
{
    kBTLeafNode       = -1,
    kBTIndexNode      =  0,
    kBTHeaderNode     =  1,
    kBTMapNode        =  2
};

struct BTHeaderRec
{
    uint16_t treeDepth;
    uint32_t rootNode;
    uint32_t leafRecords;
    uint32_t firstLeafNode;
    uint32_t lastLeafNode;
    uint16_t nodeSize;
    uint16_t maxKeyLength;
    uint32_t totalNodes;
    uint32_t freeNodes;
    uint16_t reserved1;
    uint32_t clumpSize;      // misaligned
    uint8_t btreeType;
    uint8_t keyCompareType;
    uint32_t attributes;     // long aligned again
    uint32_t reserved3[16];
} /*__attribute__((__packed__))*/;

#pragma pack(pop)

enum BTreeTypes
{
    kHFSBTreeType           =   0,      // control file
    kUserBTreeType          = 128,      // user btree type starts from 128
    kReservedBTreeType      = 255
};

enum
{
    kBTBadCloseMask           = 0x00000001,
    kBTBigKeysMask            = 0x00000002,
    kBTVariableIndexKeysMask  = 0x00000004
};

enum
{
	kHFSCaseFolding = 0xCF,
	kHFSBinaryCompare = 0xBC
};

////////////////////////////////////////////////////////////////////////////////

static void swap_device_header(DeviceHeader& x)
{
	MCDeployByteSwapRecord(true, "sslsslslss", &x, sizeof(x));
}

static void swap_device_partition(DevicePartition& x)
{
	MCDeployByteSwapRecord(true, "sslll", &x, 16);
	MCDeployByteSwapRecord(true, "llllllllll", &x . pmLgDataStart, 40);
}

static void swap_hfsplus_volume_header(HFSPlusVolumeHeader& x)
{
	MCDeployByteSwapRecord(true, "sslllllllllllllllllqllllllq", &x, sizeof(x));
	for(uint32_t i = 0; i < 5; i++)
		MCDeployByteSwapRecord(true, "qllllllllllllllllll", &x . specialFiles[i], sizeof(HFSPlusForkData));
}

static void swap_bt_node_descriptor(BTNodeDescriptor& x)
{
	MCDeployByteSwapRecord(true, "llbbss", &x, sizeof(x));
}

static void swap_bt_header_rec(BTHeaderRec& x)
{
	MCDeployByteSwapRecord(true, "sllllssllslbbll", &x, sizeof(x));
}

static void swap_hfs_unistr(HFSUniStr255& dst, const HFSUniStr255& src)
{
	dst . length = ntohs(src . length);
	for(uint32_t k = 0; k < dst . length; k++)
		dst . unicode[k] = ntohs(src . unicode[k]);
}

static void swap_hfsplus_catalog_folder(HFSPlusCatalogFolder& x)
{
	MCDeployByteSwapRecord(true, "sslllllll llbbsl ssssssss sslssl ll", &x, sizeof(x));
}

static void swap_hfsplus_catalog_file(HFSPlusCatalogFile& x)
{
	MCDeployByteSwapRecord(true, "sslllllll llbbsl llssss ssssssl ll qllllllllllllllllll qllllllllllllllllll", &x, sizeof(x));
}

////////////////////////////////////////////////////////////////////////////////

// Constructing a DMG consists of a multi-step process.
//
// The first step is to take the list of files and folders that are required
// and to construct a linear list of 'Catalog' entries that will be present in
// the filesystem. Each item generates two catalog entries - the data entry and
// the thread entry. The data entries contain the actual file/folder info while
// the thread entries generate mappings from catalog ids to items.
//
// After the list of entries is built, it is sorted and a list of data records
// built. These records are the records that get encoded into the B-Tree. This
// list of records is then turned into a B-Tree structure level-by-level, until
// a level is generated that has only one entry.
//
// 
//

// A single sector is either a reference to a 512 byte block of a file, or is
// a reference to a 512-byte block of memory. If 'data' is nil, then it is an
// empty sector.
struct DmgSector
{
	bool is_file : 1;
	unsigned offset : 31;
	void *data;
};



// A single leaf node in the Catalog BTree.
struct DmgBTreeCatalogLeaf
{
	bool thread : 1;
	unsigned index : 31;
};

struct DmgBTreeRecord
{
	uint32_t size;
	void *data;
};

struct DmgBTree
{
	// The size of each node
	uint32_t node_size;

	// The node list
	void **nodes;
	uint32_t node_count;
};

static void dmg_compute_ids(uint32_t p_index, uint32_t p_parent_index, uint32_t p_parent_parent_index, uint32_t& r_id, uint32_t& r_parent_id)
{
	if (p_index == p_parent_index)
	{
		r_id = 2;
		r_parent_id = 1;
	}
	else
	{
		r_id = 16 + p_index;
		if (p_parent_index == p_parent_parent_index)
			r_parent_id = 2;
		else
			r_parent_id = 16 + p_parent_index;
	}
}

// Split the given records up into new nodes, at the given level. The leading
// records for each node are returned in the provided buffer.
static bool dmg_btree_build_level(DmgBTree *self, uint32_t p_level, DmgBTreeRecord *p_records, uint32_t p_record_count, uint32_t *r_leaders, uint32_t& r_leader_count)
{
	// First work out roughly the number of nodes that we will need. Each record
	// requires size + sizeof(uint16_t) bytes.
	uint32_t t_total_size;
	t_total_size = 0;
	for(uint32_t i = 0; i < p_record_count; i++)
		t_total_size += p_records[i] . size + sizeof(uint16_t);

	// Now, the total size available per node is node_size - sizeof(BTNodeDescriptor)
	// so we can work out an approximate number of nodes by division.
	uint32_t t_average_node_count;
	t_average_node_count = (t_total_size / (self -> node_size - sizeof(BTNodeDescriptor))) + 1;

	// Now work out how to distribute the records amongst the nodes.
	uint32_t t_records_per_node;
	t_records_per_node = p_record_count / t_average_node_count;

	// Make sure we start out with no leaders
	r_leader_count = 0;

	// Next we just build up nodes as we traverse the record list.
	BTNodeDescriptor *t_node;
	uint32_t t_node_space, t_node_index, t_node_offset;
	t_node = nil;
	t_node_space = 0;
	t_node_index = 0;
	t_node_offset = 0;
	for(uint32_t i = 0; i < p_record_count; i++)
	{
		// If there is not enough room for this record, then create a new
		// one.
		if (t_node_space < p_records[i] . size + sizeof(uint16_t))
		{
			if (!MCMemoryReallocate(self -> nodes, (self -> node_count + 1) * sizeof(void *), self -> nodes))
				return false;
			if (!MCMemoryAllocate(self -> node_size, self -> nodes[self -> node_count]))
				return false;

			// Make sure the node starts out completely empty
			MCMemoryClear(self -> nodes[self -> node_count], self -> node_size);

			// Link in the node we are about to create, and swap the
			// node descriptor record.
			if (t_node != nil)
			{
				t_node -> fLink = t_node_index;
				swap_bt_node_descriptor(*t_node);
			}

			// Now move to the new node
			t_node = (BTNodeDescriptor *)self -> nodes[self -> node_count];
			t_node -> bLink = t_node_index;
			t_node -> kind = p_level == 1 ? kBTLeafNode : kBTIndexNode;
			t_node -> height = p_level;
			t_node -> numRecords = 0;
			t_node -> reserved = 0;
			t_node_index = self -> node_count;
			t_node_space = self -> node_size - sizeof(BTNodeDescriptor);
			t_node_offset = sizeof(BTNodeDescriptor);

			// Add the pending record to the leaders and increase the node
			// count.
			r_leaders[r_leader_count] = i;
			r_leader_count++;

			// Update the tree's node_count
			self -> node_count++;
		}

		// The node has another record
		t_node -> numRecords += 1;

		// Copy in the record data at the appropriate place
		MCMemoryCopy((uint8_t *)t_node + t_node_offset, p_records[i] . data, p_records[i] . size);

		// Add the record link at the end
		*(uint16_t *)((uint8_t *)t_node + self -> node_size  - t_node -> numRecords * sizeof(uint16_t)) = htons(t_node_offset);

		// Advance the offset, and reduce the space
		t_node_offset += p_records[i] . size;
		t_node_space -= p_records[i] . size - sizeof(uint16_t);

		// If we've reached the average records per node, set the space to zero to
		// force a new node next time.
		if (t_node -> numRecords == t_records_per_node)
			t_node_space = 0;
	}

	// Make sure we finalize the last node (if any)
	if (t_node != nil)
	{
		t_node -> fLink = 0;
		swap_bt_node_descriptor(*t_node);
	}

	return true;
}

// Sort the leaves (thread and non-thread) items in order. The key for the
// thread items is (id, ''), the key for the non-thread items is (parent id, name).
static int dmg_btree_sort_leaves(void *p_context, const void *p_a, const void *p_b)
{
	MCDeployDmgItem *t_items;
	t_items = (MCDeployDmgItem *)p_context;

	DmgBTreeCatalogLeaf *a, *b;
	a = (DmgBTreeCatalogLeaf *)p_a;
	b = (DmgBTreeCatalogLeaf *)p_b;

	uint32_t a_cid, a_parent_cid;
	dmg_compute_ids(a -> index, t_items[a -> index] . parent, t_items[t_items[a -> index] . parent] . parent, a_cid, a_parent_cid);

	uint32_t b_cid, b_parent_cid;
	dmg_compute_ids(b -> index, t_items[b -> index] . parent, t_items[t_items[b -> index] . parent] . parent, b_cid, b_parent_cid);

	uint32_t a_id, b_id;
	a_id = a -> thread ? a_cid : a_parent_cid;
	b_id = b -> thread ? b_cid : b_parent_cid;

	if (a_id != b_id)
		return a_id - b_id;

	const char *a_name, *b_name;
	a_name = a -> thread ? "" : t_items[a -> index] . name;
	b_name = b -> thread ? "" : t_items[b -> index] . name;

	return MCCStringCompare(a_name, b_name);
}

static uint32_t unix_date_to_hfs_date(uint32_t p_date)
{
	return p_date;
}

#endif

Exec_stat MCDeployDmgBuild(MCDeployDmgParameters& p_params)
{
    return ES_NORMAL;
    
#if DEPLOY_DMG
	bool t_success;
	t_success = true;

	// Open the output file
	MCDeployFileRef t_output;
	t_output = nil;
	if (t_success && !MCDeployFileOpen(p_params . output, "wb+", t_output))
		t_success = MCDeployThrow(kMCDeployErrorNoOutput);

	// First build the list of catalog leaf items we need and sort them into
	// the appropriate order.
	DmgBTreeCatalogLeaf *t_leaves;
	t_leaves = nil;
	if (t_success)
		t_success = MCMemoryNewArray(p_params . item_count * 2, t_leaves);
	if (t_success)
		for(uint32_t i = 0; i < p_params . item_count; i++)
		{
			t_leaves[i] . thread = false;
			t_leaves[i] . index = i;
			t_leaves[p_params . item_count + i] . thread = true;
			t_leaves[p_params . item_count + i] . index = i;
		}
	if (t_success)
		qsort_s(t_leaves, p_params . item_count * 2, sizeof(DmgBTreeCatalogLeaf), dmg_btree_sort_leaves, p_params . items);

	// Now we have the sorted list of leaves, we construct the leaf data records
	// that get encoded into the B-Tree.
	uint32_t t_file_count, t_folder_count;
	DmgBTreeRecord *t_leaf_records;
	t_leaf_records = nil;
	t_file_count = 0;
	t_folder_count = 0;
	if (t_success)
		t_success = MCMemoryNewArray(p_params . item_count * 2, t_leaf_records);
	if (t_success)
		for(uint32_t i = 0; i < p_params . item_count * 2 && t_success; i++)
		{
			MCDeployDmgItem *t_item;
			t_item = &p_params . items[t_leaves[i] . index];

			// If the item's parent is itself, then this is the root folder,
			// otherwise ids start 16 and up.
			uint32_t t_item_id, t_item_parent_id;
			dmg_compute_ids(t_leaves[i] . index, t_item -> parent, p_params . items[t_item -> parent] . parent, t_item_id, t_item_parent_id);

			if (t_leaves[i] . thread)
			{
				// A thread record consists of:
				//   uint16_t keyLength;
				//   uint32_t keyId;
				//   uint16_t keyName; (0 length)
				//   int16_t recordType;
				//   uint16_t reserved;
				//   uint32_t parentId;
				//   uint16_t nodeNameLength;
				//   uint16_t nodeName[nodeNameLength];
				// Thus it has size 18 + 2 * length(nodeName).

				uint32_t t_name_length;
				t_name_length = MCCStringLength(t_item -> name);

				uint8_t *t_data;
				t_leaf_records[i] . size = 18 + 2 * t_name_length;
				t_success = MCMemoryAllocate(t_leaf_records[i] . size, t_data);
				if (!t_success)
					break;

				t_leaf_records[i] . data = t_data;

				*(uint16_t *)(t_data + 0) = htons(6);
				*(uint32_t *)(t_data + 2) = htonl(t_item_id);
				*(uint16_t *)(t_data + 6) = htons(0);
				*(uint16_t *)(t_data + 8) = htons(t_item -> is_folder ? kHFSPlusFolderThreadRecord : kHFSPlusFileThreadRecord);
				*(uint16_t *)(t_data + 10) = htons(0);
				*(uint32_t *)(t_data + 12) = htonl(t_item_parent_id);
				*(uint16_t *)(t_data + 16) = htons(t_name_length);
				for(uint32_t j = 0; j < t_name_length; j++)
					*(uint16_t *)(t_data + 18 + j * 2) = htons(t_item -> name[j]);
			}
			else
			{
				// A folder records consists of:
				//   uint16_t keyLength;
				//   uint32_t parentId;
				//   uint16_t nodeNameLength;
				//   uint16_t nodeName[nodeNameLength];
				//   HFSPlusCatalogFolder/File data;

				uint32_t t_name_length;
				t_name_length = MCCStringLength(t_item -> name);

				uint8_t *t_data;
				t_leaf_records[i] . size = 8 + 2 * t_name_length + (t_item -> is_folder ? sizeof(HFSPlusCatalogFolder) : sizeof(HFSPlusCatalogFile));
				t_success = MCMemoryAllocate(t_leaf_records[i] . size, t_data);
				if (!t_success)
					break;

				t_leaf_records[i] . data = t_data;

				*(uint16_t *)(t_data + 0) = htons(6 + 2 * t_name_length);
				*(uint32_t *)(t_data + 2) = htonl(t_item_parent_id);
				*(uint16_t *)(t_data + 6) = htons(t_name_length);
				for(uint32_t j = 0; j < t_name_length; j++)
					*(uint16_t *)(t_data + 8 + j * 2) = htons(t_item -> name[j]);

				if (t_item -> is_folder)
				{
					HFSPlusCatalogFolder *t_folder;
					t_folder = (HFSPlusCatalogFolder *)(t_data + 8 + 2 * t_name_length);

					t_folder -> recordType = kHFSPlusFolderRecord;
					t_folder -> flags = 0;
					t_folder -> valence = 0;
					for(uint32_t i = 0; i < p_params . item_count; i++)
						if (p_params . items[i] . parent == t_leaves[i] . index)
							t_folder -> valence += 1;
					t_folder -> folderID = t_item_id;
					t_folder -> createDate = unix_date_to_hfs_date(t_item -> create_date);
					t_folder -> contentModDate = unix_date_to_hfs_date(t_item -> content_mod_date);
					t_folder -> attributeModDate = unix_date_to_hfs_date(t_item -> attribute_mod_date);
					t_folder -> accessDate = unix_date_to_hfs_date(t_item -> access_date);
					t_folder -> backupDate = unix_date_to_hfs_date(t_item -> backup_date);
					t_folder -> permissions . ownerID = t_item -> owner_id;
					t_folder -> permissions . groupID = t_item -> group_id;
					t_folder -> permissions . adminFlags = 0;
					t_folder -> permissions . ownerFlags = 0;
					t_folder -> permissions . fileMode = t_item -> file_mode;
					t_folder -> permissions . special . iNodeNum = 0;
					t_folder -> userInfo . windowBounds . left = t_item -> folder . window_x;
					t_folder -> userInfo . windowBounds . top = t_item -> folder . window_y;
					t_folder -> userInfo . windowBounds . right = t_item -> folder . window_x + t_item -> folder . window_width;
					t_folder -> userInfo . windowBounds . bottom = t_item -> folder . window_y + t_item -> folder . window_height;
					t_folder -> userInfo . finderFlags = 0;
					t_folder -> userInfo . location . h = t_item -> folder . location_x;
					t_folder -> userInfo . location . v = t_item -> folder . location_y;
					t_folder -> userInfo . reservedField = 0;
					t_folder -> finderInfo . scrollPosition . h = 0;
					t_folder -> finderInfo . scrollPosition . v = 0;
					t_folder -> finderInfo . reserved1 = 0;
					t_folder -> finderInfo . extendedFinderFlags = 0;
					t_folder -> finderInfo . reserved2 = 0;
					t_folder -> finderInfo . putAwayFolderID = 0;

					swap_hfsplus_catalog_folder(*t_folder);

					t_folder_count += 1;
				}
				else
				{
					HFSPlusCatalogFile *t_file;
					t_file = (HFSPlusCatalogFile *)(t_data + 8 + 2 * t_name_length);

					t_file -> recordType = kHFSPlusFileRecord;
					t_file -> flags = 0;
					t_file -> reserved1 = 0;
					t_file -> fileID = t_item_id;
					t_file -> createDate = unix_date_to_hfs_date(t_item -> create_date);
					t_file -> contentModDate = unix_date_to_hfs_date(t_item -> content_mod_date);
					t_file -> attributeModDate = unix_date_to_hfs_date(t_item -> attribute_mod_date);
					t_file -> accessDate = unix_date_to_hfs_date(t_item -> access_date);
					t_file -> backupDate = unix_date_to_hfs_date(t_item -> backup_date);
					t_file -> permissions . ownerID = t_item -> owner_id;
					t_file -> permissions . groupID = t_item -> group_id;
					t_file -> permissions . adminFlags = 0;
					t_file -> permissions . ownerFlags = 0;
					t_file -> permissions . fileMode = t_item -> file_mode;
					t_file -> permissions . special . iNodeNum = 0;
					t_file -> userInfo . fileType = t_item -> file . file_type;
					t_file -> userInfo . fileCreator = t_item -> file . file_creator;
					t_file -> userInfo . finderFlags = 0;
					t_file -> userInfo . location . h = t_item -> file . location_x;
					t_file -> userInfo . location . v = t_item -> file . location_y;
					t_file -> userInfo . reservedField = 0;
					t_file -> finderInfo . reserved1[0] = 0;
					t_file -> finderInfo . reserved1[1] = 0;
					t_file -> finderInfo . reserved1[2] = 0;
					t_file -> finderInfo . reserved1[3] = 0;
					t_file -> finderInfo . extendedFinderFlags = 0;
					t_file -> finderInfo . reserved2 = 0;
					t_file -> finderInfo . putAwayFolderID = 0;
					swap_hfsplus_catalog_file(*t_file);

					t_file_count += 1;
				}
			}
		}

	// Now we have the leaf records we can build the first level of B-Tree. Notice
	// that we allocate one node to begin with - this will be the root node. Also
	// we allocate the 'leaders' array needed for construction - this will never
	// be bigger than the number of records.
	uint32_t *t_leaders;
	DmgBTree *t_btree;
	uint32_t t_leader_count;
	t_btree = nil;
	t_leaders = nil;
	t_leader_count = 0;
	if (t_success)
		t_success =
			MCMemoryNew(t_btree) &&
			MCMemoryNewArray(1, t_btree -> nodes) &&
			MCMemoryNewArray(p_params . item_count * 2, t_leaders);
	if (t_success)
	{
		t_btree -> node_size = 4096;
		t_btree -> node_count = 1;

		// Now build the first level of the tree.
		t_success = dmg_btree_build_level(t_btree, 1, t_leaf_records, p_params . item_count * 2, t_leaders, t_leader_count);
	}

	// Next we must build the index record list for the second level of the B-Tree
	DmgBTreeRecord *t_index_records;
	uint32_t t_index_start, t_index_count;
	t_index_records = nil;
	t_index_start = t_btree -> node_count;
	t_index_count = 0;
	if (t_success)
		t_success = MCMemoryNewArray(t_leader_count, t_index_records);
	if (t_success)
	{
		for(uint32_t i = 0; i < t_leader_count && t_success; i++)
		{
			uint32_t t_key_size;
			t_key_size = ntohs(*(uint16_t *)t_leaf_records[t_leaders[i]] . data) + sizeof(uint16_t);

			uint8_t *t_data;
			t_success = MCMemoryAllocate(t_key_size + sizeof(uint32_t), t_data);
			if (!t_success)
				break;

			MCMemoryCopy(t_data, t_leaf_records[t_leaders[i]] . data, t_key_size);
			*(uint32_t *)(t_data + t_key_size) = htonl(i + 1);

			t_index_records[i] . size = t_key_size + sizeof(uint32_t);
			t_index_records[i] . data = t_data;

			t_index_count += 1;
		}
	}
	if (t_success)
		t_success = dmg_btree_build_level(t_btree, 2, t_index_records, t_leader_count, t_leaders, t_leader_count);

	// Fill in the root BTree node
	if (t_success)
		t_success = MCMemoryAllocate(t_btree -> node_size, t_btree -> nodes[0]);
	if (t_success)
	{
		MCMemoryClear(t_btree -> nodes[0], t_btree -> node_size);
		
		BTNodeDescriptor *t_head;
		t_head = (BTNodeDescriptor *)t_btree -> nodes[0];
		t_head -> fLink = 0;
		t_head -> bLink = 0;
		t_head -> kind = kBTHeaderNode;
		t_head -> height = 0;
		t_head -> numRecords = 3;
		t_head -> reserved = 0;
		swap_bt_node_descriptor(*t_head);

		BTHeaderRec *t_rec;
		t_rec = (BTHeaderRec *)((uint8_t *)t_btree -> nodes[0] + 14);
		t_rec -> treeDepth = 2;
		t_rec -> rootNode = t_index_start;
		t_rec -> leafRecords = p_params . item_count * 2;
		t_rec -> firstLeafNode = 1;
		t_rec -> lastLeafNode = t_index_start - 1;
		t_rec -> nodeSize = t_btree -> node_size;
		t_rec -> maxKeyLength = 516;
		t_rec -> totalNodes = t_btree -> node_count;
		t_rec -> freeNodes = 0;
		t_rec -> reserved1 = 0;
		t_rec -> clumpSize = 4096;
		t_rec -> btreeType = kHFSBTreeType;
		t_rec -> keyCompareType = kHFSCaseFolding;
		t_rec -> attributes = kBTBigKeysMask | kBTVariableIndexKeysMask;
		swap_bt_header_rec(*t_rec);

		memset((uint8_t *)t_btree -> nodes[0] + 14 + sizeof(BTHeaderRec) + 128, 255, t_btree -> node_size - 6 - 128 - sizeof(BTHeaderRec) - 14);

		*(uint16_t *)((uint8_t *)t_btree -> nodes[0] + t_btree -> node_size - 2) = htons(14);
		*(uint16_t *)((uint8_t *)t_btree -> nodes[0] + t_btree -> node_size - 4) = htons(14 + sizeof(BTHeaderRec));
		*(uint16_t *)((uint8_t *)t_btree -> nodes[0] + t_btree -> node_size - 6) = htons(14 + sizeof(BTHeaderRec) + 128);
	}

	// Create the root Extents BTree node
	char t_extents[4096];
	if (t_success)
	{
		MCMemoryClear(t_extents, 4096);

		BTNodeDescriptor *t_head;
		t_head = (BTNodeDescriptor *)t_extents;
		t_head -> fLink = 0;
		t_head -> bLink = 0;
		t_head -> kind = kBTHeaderNode;
		t_head -> height = 0;
		t_head -> numRecords = 3;
		t_head -> reserved = 0;
		swap_bt_node_descriptor(*t_head);

		BTHeaderRec *t_rec;
		t_rec = (BTHeaderRec *)(t_extents + 14);
		t_rec -> treeDepth = 0;
		t_rec -> rootNode = 0;
		t_rec -> leafRecords = 0;
		t_rec -> firstLeafNode = 0;
		t_rec -> lastLeafNode = 0;
		t_rec -> nodeSize = 4096;
		t_rec -> maxKeyLength = 10;
		t_rec -> totalNodes = 1;
		t_rec -> freeNodes = 0;
		t_rec -> reserved1 = 0;
		t_rec -> clumpSize = 4096;
		t_rec -> btreeType = kHFSBTreeType;
		t_rec -> keyCompareType = 0;
		t_rec -> attributes = kBTBigKeysMask;
		swap_bt_header_rec(*t_rec);

		memset((uint8_t *)t_extents + 14 + sizeof(BTHeaderRec) + 128, 255, 4096 - 6 - 128 - sizeof(BTHeaderRec) - 14);

		*(uint16_t *)(t_extents + 4096 - 2) = htons(14);
		*(uint16_t *)(t_extents + 4096 - 4) = htons(14 + sizeof(BTHeaderRec));
		*(uint16_t *)(t_extents + 4096 - 6) = htons(14 + sizeof(BTHeaderRec) + 128);
	}

	// Create the root Attributes BTree node
	char t_attrs[8192];
	if (t_success)
	{
		MCMemoryClear(t_attrs, 8192);

		BTNodeDescriptor *t_head;
		t_head = (BTNodeDescriptor *)t_attrs;
		t_head -> fLink = 0;
		t_head -> bLink = 0;
		t_head -> kind = kBTHeaderNode;
		t_head -> height = 0;
		t_head -> numRecords = 3;
		t_head -> reserved = 0;
		swap_bt_node_descriptor(*t_head);

		BTHeaderRec *t_rec;
		t_rec = (BTHeaderRec *)(t_attrs + 14);
		t_rec -> treeDepth = 0;
		t_rec -> rootNode = 0;
		t_rec -> leafRecords = 0;
		t_rec -> firstLeafNode = 0;
		t_rec -> lastLeafNode = 0;
		t_rec -> nodeSize = 8192;
		t_rec -> maxKeyLength = 266;
		t_rec -> totalNodes = 1;
		t_rec -> freeNodes = 0;
		t_rec -> reserved1 = 0;
		t_rec -> clumpSize = 4096;
		t_rec -> btreeType = 255;
		t_rec -> keyCompareType = 188;
		t_rec -> attributes = 6;
		swap_bt_header_rec(*t_rec);

		memset((uint8_t *)t_attrs + 14 + sizeof(BTHeaderRec) + 128, 255, 8192 - 6 - 128 - sizeof(BTHeaderRec) - 14);

		*(uint16_t *)(t_attrs + 8192 - 2) = htons(14);
		*(uint16_t *)(t_attrs + 8192 - 4) = htons(14 + sizeof(BTHeaderRec));
		*(uint16_t *)(t_attrs + 8192 - 6) = htons(14 + sizeof(BTHeaderRec) + 128);
	}

	// The output image consists of:
	//   [ 0 ] Device header block
	//   [ 1 ] Partition 0: 'Apple_partition_map'
	//   [ 2 ] Partition 1: 'Apple_HFS'
	//   [ 3 ] Partition 2: 'Apple_Free'
	//   [ 4..9 ] HFS: Reserved + Volume Header
	//   [ **** ] HFS: Data blocks
	//   [ **** ] HFS: Catalog File
	//   [ **** ] HFS: Allocation File
	//   [ **** ] HFS: Alternative Volume Header
	//   [ **** ] HFS: Reserved (1 sector)

	// Work out the total allocation blocks used by the HFS filesystem
	uint32_t t_block_count, t_alloc_block_count;
	t_block_count = 0;
	t_alloc_block_count = 0;
	if (t_success)
	{
		// The volume header and reserved section take up the first block
		t_block_count += 1;
		
		// The Catalog BTree uses one allocation block per node
		t_block_count += t_btree -> node_count;

		// The Extents BTree
		t_block_count += 1;

		// The Attributes BTree
		t_block_count += 2;

		// The alternative volume header/terminal reserved section needs one block
		t_block_count += 1;

		// The allocation file needs one block for every 32768 blocks
		t_alloc_block_count =  (t_block_count + 32767) / 32768 + 1;
		t_block_count += t_alloc_block_count;
	}

	// Compute the sector count
	uint32_t t_sector_count;
	t_sector_count = 0;
	if (t_success)
	{
		t_sector_count += 1; // Device header
		t_sector_count += 7; // Partition map
		t_sector_count += t_block_count * 8; // HFS
		t_sector_count += 1; // Free sector
	}

	// Allocate the sectors
	DmgSector *t_sectors;
	t_sectors = nil;
	if (t_success)
		t_success = MCMemoryNewArray(t_sector_count, t_sectors);

	char t_empty[512];
	char t_full[512];
	DeviceHeader t_device_header;
	DevicePartition t_device_partitions[4];
	HFSPlusVolumeHeader t_volume_header;
	if (t_success)
	{
		MCMemoryClear(t_empty, sizeof(t_empty));
		memset(t_full, 255, sizeof(t_full));

		MCMemoryClear(&t_device_header, sizeof(DeviceHeader));
		t_device_header . sbSig = kDeviceHeaderSignature;
		t_device_header . sbBlkSize = 512;
		t_device_header . sbBlkCount = t_sector_count;
		swap_device_header(t_device_header);

		MCMemoryClear(&t_device_partitions[0], sizeof(DevicePartition));
		t_device_partitions[0] . pmSig = kDevicePartitionSignature;
		t_device_partitions[0] . pmMapBlkCnt = 3;
		t_device_partitions[0] . pmPyPartStart = 1;
		t_device_partitions[0] . pmPartBlkCnt = 7;
		MCMemoryCopy(&t_device_partitions[0] . pmPartName[0], "Apple", 5);
		MCMemoryCopy(&t_device_partitions[0] . pmParType[0], "Apple_partition_map", 19);
		swap_device_partition(t_device_partitions[0]);
		
		MCMemoryClear(&t_device_partitions[1], sizeof(DevicePartition));
		t_device_partitions[1] . pmSig = kDevicePartitionSignature;
		t_device_partitions[1] . pmMapBlkCnt = 3;
		t_device_partitions[1] . pmPyPartStart = 8;
		t_device_partitions[1] . pmPartBlkCnt = t_block_count * 8;
		MCMemoryCopy(&t_device_partitions[1] . pmPartName[0], "disk image", 10);
		MCMemoryCopy(&t_device_partitions[1] . pmParType[0], "Apple_HFS", 9);
		swap_device_partition(t_device_partitions[1]);

		MCMemoryClear(&t_device_partitions[2], sizeof(DevicePartition));
		t_device_partitions[2] . pmSig = kDevicePartitionSignature;
		t_device_partitions[2] . pmMapBlkCnt = 3;
		t_device_partitions[2] . pmPyPartStart = t_sector_count - 1;
		t_device_partitions[2] . pmPartBlkCnt = 1;
		MCMemoryCopy(&t_device_partitions[2] . pmPartName[0], "", 0);
		MCMemoryCopy(&t_device_partitions[2] . pmParType[0], "Apple_Free", 10);
		swap_device_partition(t_device_partitions[2]);

		MCMemoryClear(&t_volume_header, sizeof(HFSPlusVolumeHeader));
		t_volume_header . signature = 'H+';
		t_volume_header . version = 4;
		t_volume_header . attributes = 1 << kHFSVolumeUnmountedBit;
		t_volume_header . lastMountedVersion = '10.0';
		t_volume_header . journalInfoBlock = 0;
		t_volume_header . createDate = 0;
		t_volume_header . modifyDate = 0;
		t_volume_header . backupDate = 0;
		t_volume_header . checkedDate = 0;
		t_volume_header . fileCount = t_file_count;
		t_volume_header . folderCount = t_folder_count;
		t_volume_header . blockSize = 4096;
		t_volume_header . totalBlocks = t_block_count;
		t_volume_header . freeBlocks = 0;
		t_volume_header . nextAllocation = 0;
		t_volume_header . rsrcClumpSize = 65536;
		t_volume_header . dataClumpSize = 65536;
		t_volume_header . nextCatalogID = 16 + p_params . item_count + 1;
		t_volume_header . writeCount = 1;
		t_volume_header . encodingsBitmap = 1;

		uint32_t t_alloc_start;
		t_alloc_start = t_block_count - t_alloc_block_count - 1;
		t_volume_header . allocationFile . clumpSize = t_alloc_block_count * 4096;
		t_volume_header . allocationFile . logicalSize = t_alloc_block_count * 4096;
		t_volume_header . allocationFile . totalBlocks = t_alloc_block_count;
		t_volume_header . allocationFile . extents[0] . startBlock = t_alloc_start;
		t_volume_header . allocationFile . extents[0] . blockCount = t_alloc_block_count;

		uint32_t t_catalog_start;
		t_catalog_start = t_alloc_start - t_btree -> node_count;
		t_volume_header . catalogFile . clumpSize = t_btree -> node_count * 4096;
		t_volume_header . catalogFile . logicalSize = t_btree -> node_count * 4096;
		t_volume_header . catalogFile . totalBlocks = t_btree -> node_count;
		t_volume_header . catalogFile . extents[0] . startBlock = t_catalog_start;
		t_volume_header . catalogFile . extents[0] . blockCount = t_btree -> node_count;

		uint32_t t_extents_start;
		t_extents_start = t_catalog_start - 1;
		t_volume_header . extentsFile . clumpSize = 4096;
		t_volume_header . extentsFile . logicalSize = 4096;
		t_volume_header . extentsFile . totalBlocks = 1;
		t_volume_header . extentsFile . extents[0] . startBlock = t_extents_start;
		t_volume_header . extentsFile . extents[0] . blockCount = 1;

		uint32_t t_attrs_start;
		t_attrs_start = t_extents_start - 2;
		t_volume_header . attributesFile . clumpSize = 8192;
		t_volume_header . attributesFile . logicalSize = 8192;
		t_volume_header . attributesFile . totalBlocks = 2;
		t_volume_header . attributesFile . extents[0] . startBlock = t_attrs_start;
		t_volume_header . attributesFile . extents[0] . blockCount = 2;

		swap_hfsplus_volume_header(t_volume_header);

		// Now setup the sectors array
		t_sectors[0] . data = &t_device_header;
		t_sectors[1] . data = &t_device_partitions[0];
		t_sectors[2] . data = &t_device_partitions[1];
		t_sectors[3] . data = &t_device_partitions[2];

		// HFS: Reserved
		t_sectors[8] . data = t_empty;
		t_sectors[9] . data = t_empty;
		// HFS: Volume Header
		t_sectors[10] . data = &t_volume_header;

		// HFS: Data

		// HFS: Catalog File
		for(uint32_t i = 0; i < t_btree -> node_count; i++)
		{
			uint32_t s;
			s = 8 + t_catalog_start * 8 + i * 8;
			for(uint32_t j = 0; j < 8; j++)
			{
				t_sectors[s + j] . data = t_btree -> nodes[i];
				t_sectors[s + j] . offset = j * 512;
			}
		}

		// HFS: Allocation File
		for(uint32_t i = 0; i < t_alloc_block_count; i++)
		{
			uint32_t s;
			s = 8 + t_alloc_start * 8 + i * 8;
			for(uint32_t j = 0; j < 8; j++)
				t_sectors[s + j] . data = t_full;
		}

		for(uint32_t j = 0; j < 8; j++)
		{
			t_sectors[8 + t_extents_start * 8 + j] . data = t_extents;
			t_sectors[8 + t_extents_start * 8 + j] . offset = j * 512;
		}

		for(uint32_t j = 0; j < 16; j++)
		{
			t_sectors[8 + t_attrs_start * 8 + j] . data = t_attrs;
			t_sectors[8 + t_attrs_start * 8 + j] . offset = j * 512;
		}

		// HFS: Alternate volume header
		t_sectors[8 + t_block_count * 8 - 2] . data = &t_volume_header;
		// HFS: Reserved
		t_sectors[8 + t_block_count * 8 - 1] . data = t_empty;

		// Free sector
		t_sectors[t_sector_count - 1] . data = t_empty;

		////////

		for(uint32_t i = 0; i < t_sector_count && t_success; i++)
		{
			if (!t_sectors[i] . is_file)
			{
				if (t_sectors[i] . data != nil)
					t_success = MCDeployFileWriteAt(t_output, (uint8_t *)t_sectors[i] . data + t_sectors[i] . offset, 512, i * 512);
			}
		}
	}

	if (t_btree != nil)
	{
		for(uint32_t i = 0; i < t_btree -> node_count; i++)
			MCMemoryDeallocate(t_btree -> nodes[i]);
		MCMemoryDeleteArray(t_btree -> nodes);
		MCMemoryDelete(t_btree);
	}
	if (t_index_records != nil)
	{
		for(uint32_t i = 0; i < t_index_count; i++)
			MCMemoryDeallocate(t_index_records[i] . data);
		MCMemoryDeleteArray(t_index_records);
	}
	MCMemoryDeleteArray(t_leaders);
	if (t_leaf_records != nil)
	{
		for(uint32_t i = 0; i < p_params . item_count * 2; i++)
			MCMemoryDeallocate(t_leaf_records[i] . data);
		MCMemoryDeleteArray(t_leaf_records);
	}
	MCMemoryDeleteArray(t_leaves);

	MCDeployFileClose(t_output);
    
	return t_success ? ES_NORMAL : ES_ERROR;
#endif
}

////////////////////////////////////////////////////////////////////////////////

typedef void (*log_callback)(void *, const char *, ...);

#if DEPLOY_DMG
static bool read_hfsplus_fork(MCDeployFileRef p_file, uint32_t p_offset, HFSPlusForkData& p_fork, uint8_t*& r_data)
{
	uint8_t *t_data;
	if (!MCMemoryAllocate((uint32_t)p_fork . logicalSize, t_data))
		return false;

	if (!MCDeployFileReadAt(p_file, t_data, (uint32_t)p_fork . logicalSize, p_offset + p_fork . extents[0] . startBlock * 4096))
	{
		MCMemoryDeallocate(t_data);
		return false;
	}

	r_data = t_data;

	return true;
}

static void log_hfsplus_fork_data(log_callback p_log, void *p_context, const char *p_name, HFSPlusForkData *p_data)
{
	p_log(p_context, "%s:\n", p_name);
	p_log(p_context, "  logicalSize = %llu\n", p_data -> logicalSize);
	p_log(p_context, "  clumpSize = %u\n", p_data -> clumpSize);
	p_log(p_context, "  totalBlocks = %u\n", p_data -> totalBlocks);
	for(uint32_t i = 0; i < 7; i++)
	{
		if (p_data -> extents[i] . blockCount == 0)
			break;
		p_log(p_context, "  extents[%u].start = %u\n", i, p_data -> extents[i] . startBlock);
		p_log(p_context, "  extents[%u].count = %u\n", i, p_data -> extents[i] . blockCount);
	}
}

static void log_btree(log_callback p_log, void *p_context, const char *p_name, bool p_is_catalog, uint8_t *p_data)
{
	p_log(p_context, "%s:\n", p_name);

	BTNodeDescriptor t_header;
	t_header = *(BTNodeDescriptor *)p_data;
	swap_bt_node_descriptor(t_header);

	BTHeaderRec t_header_rec;
	t_header_rec = *(BTHeaderRec *)(p_data + 14);
	swap_bt_header_rec(t_header_rec);

	p_log(p_context, "  treeDepth = %u\n", t_header_rec . treeDepth);
	p_log(p_context, "  rootNode = %u\n", t_header_rec . rootNode);
	p_log(p_context, "  leafRecords = %u\n", t_header_rec . leafRecords);
	p_log(p_context, "  firstLeafNode = %u\n", t_header_rec . firstLeafNode);
	p_log(p_context, "  lastLeafNode = %u\n", t_header_rec . lastLeafNode);
	p_log(p_context, "  nodeSize = %u\n", t_header_rec . nodeSize);
	p_log(p_context, "  maxKeyLength = %u\n", t_header_rec . maxKeyLength);
	p_log(p_context, "  totalNodes = %u\n", t_header_rec . totalNodes);
	p_log(p_context, "  freeNodes = %u\n", t_header_rec . freeNodes);
	p_log(p_context, "  clumpSize = %u\n", t_header_rec . clumpSize);
	p_log(p_context, "  btreeType = %u\n", t_header_rec . btreeType);
	p_log(p_context, "  keyCompareType = %u\n", t_header_rec . keyCompareType);
	p_log(p_context, "  attributes = %u\n", t_header_rec . attributes);

	for(uint32_t i = 0; i < t_header_rec . totalNodes; i++)
	{
		uint8_t *t_node_ptr;
		t_node_ptr = p_data + t_header_rec . nodeSize * i;

		BTNodeDescriptor t_node;
		t_node = *(BTNodeDescriptor *)t_node_ptr;
		swap_bt_node_descriptor(t_node);

		if (t_node . numRecords != 0)
		{
			p_log(p_context, "  node[%u]:\n", i);
			p_log(p_context, "    fLink = %u\n", t_node . fLink);
			p_log(p_context, "    bLink = %u\n", t_node . bLink);
			p_log(p_context, "    kind = %d\n", t_node . kind);
			p_log(p_context, "    height = %u\n", t_node . height);
			p_log(p_context, "    numRecords = %u\n", t_node . numRecords);

			for(uint32_t j = 0; j < t_node . numRecords; j++)
			{
				uint16_t t_rec_offset;
				t_rec_offset = ntohs(*(uint16_t *)(t_node_ptr + t_header_rec . nodeSize - (j + 1) * sizeof(uint16_t)));
				p_log(p_context, "    record[%u]:\n", j);
				p_log(p_context, "      offset = %u\n", t_rec_offset);
				if (p_is_catalog &&
					(t_node . kind == kBTLeafNode || t_node . kind == kBTIndexNode))
				{
					uint32_t t_key_length;
					uint8_t *t_key, *t_data;

					t_key_length = ntohs(*(uint16_t *)(t_node_ptr + t_rec_offset));
					t_key = t_node_ptr + t_rec_offset + sizeof(uint16_t);

					if (t_node . kind != kBTIndexNode || (t_header_rec . attributes & kBTVariableIndexKeysMask) != 0)
						t_data = t_key + t_key_length;
					else
						t_data = t_key + t_header_rec . maxKeyLength;

					uint32_t t_key_id;
					HFSUniStr255 t_key_name;
					t_key_id = ntohl(*(uint32_t *)t_key);
					swap_hfs_unistr(t_key_name, *(HFSUniStr255 *)(t_key + 4));

					if (t_node . kind == kBTIndexNode)
					{
						p_log(p_context, "      parentId = %u\n", t_key_id);
						p_log(p_context, "      nodeName = '%.*S'\n", t_key_name . length, t_key_name . unicode);
						p_log(p_context, "      childNode = %u\n", ntohl(*(uint32_t *)t_data));
					}
					else
					{
						uint32_t t_data_type;
						t_data_type = ntohs(*(uint16_t *)t_data);
						switch(t_data_type)
						{
						case kHFSPlusFolderRecord:
							p_log(p_context, "      type = folder\n");
							p_log(p_context, "      parentId = %u\n", t_key_id);
							p_log(p_context, "      nodeName = '%.*S'\n", t_key_name . length, t_key_name . unicode);
							break;
						case kHFSPlusFileRecord:
							p_log(p_context, "      type = file\n");
							p_log(p_context, "      parentId = %u\n", t_key_id);
							p_log(p_context, "      nodeName = '%.*S'\n", t_key_name . length, t_key_name . unicode);
							break;
						case kHFSPlusFolderThreadRecord:
						case kHFSPlusFileThreadRecord:
							{
								HFSUniStr255 t_node_name;
								swap_hfs_unistr(t_node_name, *(HFSUniStr255 *)(t_data + 8));
								uint32_t t_parent_id;
								t_parent_id = ntohl(*(uint32_t *)(t_data + 4));
								p_log(p_context, "      type = %s thread\n", t_data_type == kHFSPlusFolderThreadRecord ? "folder" : "file");
								p_log(p_context, "      folderId = %u\n", t_key_id);
								p_log(p_context, "      parentId = %u\n", t_parent_id);
								p_log(p_context, "      nodeName = '%.*S'\n", t_node_name . length, t_node_name . unicode);
							}
							break;
						}
					}
				}
			}
		}
	}
}
#endif

bool MCDeployDmgDump(const char *p_dmg_file, log_callback p_log, void *p_context)
{
	bool t_success;
	t_success = true;

#if DEPLOY_DMG
	MCDeployFileRef t_dmg;
	t_dmg = nil;
	if (t_success &&
		!MCDeployFileOpen(p_dmg_file, "rb", t_dmg))
		t_success = MCDeployThrow(kMCDeployErrorBadFile);

	// The device header occupies the first 512-byte block
	DeviceHeader t_dev_header;
	if (t_success)
		t_success = MCDeployFileReadAt(t_dmg, &t_dev_header, sizeof(DeviceHeader), 0);
	if (t_success)
	{
		swap_device_header(t_dev_header);
		p_log(p_context, "Device header:\n");
		p_log(p_context, "  signature = '%.2s'\n", &t_dev_header . sbSig);
		p_log(p_context, "  block size = %u\n", t_dev_header . sbBlkSize);
		p_log(p_context, "  block count = %u\n", t_dev_header . sbBlkCount);
		p_log(p_context, "\n");
	}

	DevicePartition t_dev_part;
	uint32_t t_dmg_offset, t_dmg_size;
	t_dmg_offset = 0;
	t_dmg_size = 0;
	if (t_success)
		for(uint32_t i = 0; true; i++)
		{
			t_success = MCDeployFileReadAt(t_dmg, &t_dev_part, sizeof(DevicePartition), (i + 1) * t_dev_header . sbBlkSize);
			if (!t_success)
				break;

			swap_device_partition(t_dev_part);

			if (t_dev_part . pmSig != kDevicePartitionSignature)
			{
				if (i + 1 == t_dev_part . pmMapBlkCnt)
					break;

				continue;
			}

			p_log(p_context, "Device partition %u:\n", i);
			p_log(p_context, "  signature = '%.2s'\n", &t_dev_part . pmSig);
			p_log(p_context, "  start block = %u\n", t_dev_part . pmPyPartStart);
			p_log(p_context, "  block count = %u\n", t_dev_part . pmPartBlkCnt);
			p_log(p_context, "  name = '%.32s'\n", t_dev_part . pmPartName);
			p_log(p_context, "  type = '%.32s'\n", t_dev_part . pmParType);
			p_log(p_context, "  start data = %u\n", t_dev_part . pmLgDataStart);
			p_log(p_context, "  data count = %u\n", t_dev_part . pmDataCnt);
			p_log(p_context, "\n");

			if (MCCStringEqual((const char *)t_dev_part . pmPartName, "disk image"))
			{
				t_dmg_offset = t_dev_part . pmPyPartStart * t_dev_header . sbBlkSize;
				t_dmg_size = t_dev_part . pmPartBlkCnt * t_dev_header . sbBlkSize;
			}

			if (i + 1 == t_dev_part . pmMapBlkCnt)
				break;
		}

	HFSPlusVolumeHeader t_header;
	if (t_success)
		t_success = MCDeployFileReadAt(t_dmg, &t_header, sizeof(t_header), t_dmg_offset + 1024);

	if (t_success)
	{
		swap_hfsplus_volume_header(t_header);
		p_log(p_context, "Volume header:\n");
		p_log(p_context, "  signature = '%.2s'\n", &t_header . signature);
		p_log(p_context, "  version = %d\n", t_header . version);
		p_log(p_context, "  attributes = %08x\n", t_header . attributes);
		p_log(p_context, "  lastMountedVersion = '%.4s'\n", &t_header . lastMountedVersion);
		p_log(p_context, "  journalInfoBlock = %08x\n", t_header . journalInfoBlock);
		p_log(p_context, "  createDate = %u\n", t_header . createDate);
		p_log(p_context, "  modifyDate = %u\n", t_header . modifyDate);
		p_log(p_context, "  backupDate = %u\n", t_header . backupDate);
		p_log(p_context, "  checkedDate = %u\n", t_header . checkedDate);
		p_log(p_context, "  fileCount = %u\n", t_header . fileCount);
		p_log(p_context, "  folderCount = %u\n", t_header . folderCount);
		p_log(p_context, "  blockSize = %u\n", t_header . blockSize);
		p_log(p_context, "  totalBlocks = %u\n", t_header . totalBlocks);
		p_log(p_context, "  freeBlocks = %u\n", t_header . freeBlocks);
		p_log(p_context, "  nextAllocation = %u\n", t_header . nextAllocation);
		p_log(p_context, "  rsrcClumpSize = %u\n", t_header . rsrcClumpSize);
		p_log(p_context, "  dataClumpSize = %u\n", t_header . dataClumpSize);
		p_log(p_context, "  nextCatalogID = %u\n", t_header . nextCatalogID);
		p_log(p_context, "  writeCount = %u\n", t_header . writeCount);
		p_log(p_context, "  encodingsBitmap = %u\n", t_header . encodingsBitmap);
		p_log(p_context, "  finderInfo.bootDirId = %u\n", t_header . finderInfo[0]);
		p_log(p_context, "  finderInfo.parentDirId = %u\n", t_header . finderInfo[1]);
		p_log(p_context, "  finderInfo.windowDirId = %u\n", t_header . finderInfo[2]);
		p_log(p_context, "  finderInfo.classicDirId = %u\n", t_header . finderInfo[3]);
		p_log(p_context, "  finderInfo.reserved = %u\n", t_header . finderInfo[4]);
		p_log(p_context, "  finderInfo.macosxDirId = %u\n", t_header . finderInfo[5]);
		p_log(p_context, "  finderInfo.uniqueId = %016llx\n", t_header . finderInfo[6], t_header . finderInfo[7]);

		log_hfsplus_fork_data(p_log, p_context, "allocationFile", &t_header . allocationFile);
		log_hfsplus_fork_data(p_log, p_context, "extentsFile", &t_header . extentsFile);
		log_hfsplus_fork_data(p_log, p_context, "catalogFile", &t_header . catalogFile);
		log_hfsplus_fork_data(p_log, p_context, "attributesFile", &t_header . attributesFile);
		log_hfsplus_fork_data(p_log, p_context, "startupFile", &t_header . startupFile);
		p_log(p_context, "\n");
	}

	if (t_success)
	{
		uint8_t *t_alloc_map;
		t_success = read_hfsplus_fork(t_dmg, t_dmg_offset, t_header . allocationFile, t_alloc_map);
		if (t_success)
		{
			p_log(p_context, "Allocation map:\n");
			for(uint32_t i = 0; i < (t_header . totalBlocks + 7) / 8; i += 32)
			{
				p_log(p_context, "  ");
				for(uint32_t j = 0; j < 32; j++)
					p_log(p_context, "%02x", t_alloc_map[i + j]);
				p_log(p_context, "\n");
			}
			p_log(p_context, "\n");
			MCMemoryDeallocate(t_alloc_map);
		}
	}

	if (t_success && t_header . extentsFile . logicalSize > 0)
	{
		uint8_t *t_extents;
		t_success = read_hfsplus_fork(t_dmg, t_dmg_offset, t_header . extentsFile, t_extents);
		if (t_success)
		{
			log_btree(p_log, p_context, "Extents B-Tree", false, t_extents);
			p_log(p_context, "\n");
			MCMemoryDeallocate(t_extents);
		}
	}

	if (t_success && t_header . catalogFile . logicalSize > 0)
	{
		uint8_t *t_catalog;
		t_success = read_hfsplus_fork(t_dmg, t_dmg_offset, t_header . catalogFile, t_catalog);
		if (t_success)
		{
			log_btree(p_log, p_context, "Catalog B-Tree", true, t_catalog);
			p_log(p_context, "\n");
			MCMemoryDeallocate(t_catalog);
		}
	}

	if (t_success && t_header . attributesFile . logicalSize > 0)
	{
		uint8_t *t_data;
		t_success = read_hfsplus_fork(t_dmg, t_dmg_offset, t_header . attributesFile, t_data);
		if (t_success)
		{
			log_btree(p_log, p_context, "Attributes B-Tree", false, t_data);
			p_log(p_context, "\n");
			MCMemoryDeallocate(t_data);
		}
	}
#endif

	return true;
}

////////////////////////////////////////////////////////////////////////////////
