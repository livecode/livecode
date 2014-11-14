#ifndef __MC_MAC_PLAYER__
#define __MC_MAC_PLAYER__

////////////////////////////////////////////////////////////////////////////////

class MCPlatformPlayer
{
public:
	MCPlatformPlayer(void);
	virtual ~MCPlatformPlayer(void);
    
	void Retain(void);
	void Release(void);
    
	void Attach(MCPlatformWindowRef window);
	void Detach(void);
    
	virtual bool IsPlaying(void) = 0;
    // PM-2014-05-28: [[ Bug 12523 ]] Take into account the playRate property
	virtual void Start(double rate) = 0;
	virtual void Stop(void) = 0;
	virtual void Step(int amount) = 0;
    
	virtual void LockBitmap(MCImageBitmap*& r_bitmap) = 0;
	virtual void UnlockBitmap(MCImageBitmap *bitmap) = 0;
    
	virtual void SetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value) = 0;
	virtual void GetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value) = 0;
    
	virtual void CountTracks(uindex_t& r_count) = 0;
	virtual bool FindTrackWithId(uint32_t id, uindex_t& r_index) = 0;
	virtual void SetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value) = 0;
	virtual void GetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value) = 0;
    
protected:
	virtual void Realize(void) = 0;
	virtual void Unrealize(void) = 0;
    
	static void DoWindowStateChanged(void *object, bool realized);
    
protected:
	uint32_t m_references;
    
	MCPlatformWindowRef m_window;
    
};

////////////////////////////////////////////////////////////////////////////////

#endif
