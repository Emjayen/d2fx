/*
 * d2def.h
 *
 */
#ifndef D2DEF_H
#define D2DEF_H
#include "pce\pce.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define D2FUNC(_addr, _ret, _abi, _name, ...) _ret (_abi* static const _name)(__VA_ARGS__) = (_ret (_abi*)(__VA_ARGS__)) _addr;



/*
struct D2DynamicPathStrc
{
	union
	{
		struct
		{
			uint16_t wOffsetX;                    //0x00
			uint16_t wPosX;                        //0x02
			uint16_t wOffsetY;                    //0x04
			uint16_t wPosY;                        //0x06
		};
		struct
		{
			uint32_t dwPrecisionX;                //0x00
			uint32_t dwPrecisionY;                //0x04
		};
	};
	union
	{
		struct
		{
			uint32_t dwTargetX;                    //0x08
			uint32_t dwTargetY;                    //0x0C
		};
		D2CoordStrc cPrecisionTarget;            //0x08
	};
	union
	{
		struct
		{
			uint16_t xSP1;                                //0x10
			uint16_t ySP1;                                //0x12
		};
		D2PathPointStrc cTarget;                //0x10
	};
	union
	{
		struct
		{
			uint16_t xSP2;                                //0x14
			uint16_t ySP2;                                //0x16
			uint16_t xSP3;                                //0x18
			uint16_t ySP3;                                //0x1A
		};
		D2PathPointStrc cTargetEX[2];            //0x14
	};
	D2RoomStrc* pRoom;                        //0x1C
	D2RoomStrc* pRoomNext;                    //0x20
	int32_t unk0x24;                            //0x24
	int32_t dwPathPoints;                        //0x28
	void* unk0x2C;                            //0x2C
	D2UnitStrc* pUnit;                        //0x30
	uint32_t dwFlags;                            //0x34
	union
	{
		uint32_t unk0x38;                        //0x38
		uint32_t dwPathTypeEx;                    //0x38
	};
	uint32_t dwPathType;                        //0x3C
	uint32_t dwPrevPathType;                    //0x40
	uint32_t dwUnitSize;                        //0x44
	uint32_t dwCollisionPattern;                //0x48
	uint32_t dwCollisionType;                    //0x4C
	union
	{
		uint32_t unk0x50;                            //0x50
		uint32_t dwPathIndex;                    //0x50
	};
	union
	{
		uint16_t unk0x54;                            //0x54
		uint16_t fAIFlags;                        //0x54
	};
	uint16_t unk0x56;                            //0x56
	D2UnitStrc* pTargetUnit;                //0x58
	uint32_t dwTargetType;                        //0x5C
	uint32_t dwTargetId;                        //0x60
	uint8_t nDirection;                        //0x64
	uint8_t nNewDirection;                        //0x65
	uint8_t nDiffDirection;                    //0x66
	uint8_t unk0x67;                            //0x67
	uint8_t unk0x68[10];                        //0x68
	union
	{
		struct
		{
			int32_t unk0x72;                            //0x72
			int32_t unk0x76;                            //0x76
		};
		D2CoordStrc cAccel;                    //0x72
	};
	char unk0x7A[2];                        //0x7A
	uint32_t dwVelocity;                        //0x7C
	union
	{
		uint32_t unk0x80;                            //0x80
		uint32_t dwPrevVelocity;                //0x80
	};
	union
	{
		uint32_t dwVelocity2;                        //0x84
		uint32_t dwMaxVelocity;                    //0x84
	};
	union
	{
		struct
		{
			uint32_t unk0x88[2];                        //0x88
		};
		struct
		{
			uint32_t dwMissileVelocity;                //0x88
			uint32_t _8;                               //0x8C
		};
	};
	uint8_t nDist;                                //0x90
	uint8_t nDistMax;                            //0x91
	union
	{
		uint8_t unk0x92;                            //0x92
		uint8_t nDistMaxAlt;                    //0x92
	};
	uint8_t nStepNum;                            //0x93
	union
	{
		struct
		{
			uint8_t nDistance;                            //0x94
			char unk0x95[3];                        //0x95
		};
		int nDistEx;                        //0x94
	};
	int nValueType;                            //0x98
	D2PathPointStrc PathPoints[78];            //0x9C
	int32_t nPathPointsEx;                            //0x1D4
	D2PathPointStrc pPathPointsEx[10];            //0x1D8
};

*/



// Forwards
struct D2Game;
struct D2Unit;


#pragma pack(1)

struct D2Inventory
{
	DWORD dwSignature;		//0x00
	void* pMemPool;			//0x04
	D2Unit* pOwner;			//0x08
	D2Unit* pFirstItem;		//0x0C
	D2Unit* pLastItem;		//0x10
	void* pInvInfo;			//0x14
	int nInvInfo;			//0x18
	DWORD dwLeftItemGUID;		//0x1C
	D2Unit* pCursorItem;		//0x20
	DWORD dwOwnerId;		//0x24
	DWORD dwItemCount;		//0x28
	DWORD unk0x2C[2];		//0x2C
	void* pFirstCorpse;		//0x34
	DWORD CorpseNumber;		//0x38
};

struct pathpoint
{
	WORD X;
	WORD Y;
};

struct stat
{
	union
	{
		DWORD    Stat;         //+00

		struct
		{
			WORD _Param;        //+00   
			WORD _Stat;         //+02
		};
	};

	DWORD Value;               //+04
};

struct statex
{
	stat* pStat;				//0x00 An Array[wStatCount]
	WORD wStatCount;			//0x04
	WORD wnBitSize;				//0x06
};

struct statlist
{
	void* pMemPool;			//0x00
	D2Unit* pUnit;			//0x04
	DWORD dwOwnerType;		//0x08
	DWORD dwOwnerId;		//0x0C
	DWORD dwFlags;			//0x10
	DWORD dwStateNo;		//0x14
	DWORD dwExpireFrame;		//0x18
	DWORD dwSkillNo;		//0x1C
	DWORD dwSLvl;			//0x20
	statex Stats;			//0x24
	statlist* pPrevLink;		//0x2C
	statlist* pNextLink;		//0x30
	statlist* pPrev;		//0x34
	void* fpStatRemove;		//0x38
	statlist* pNext;		//0x3C
	statlist* pSetList;		//0x40
	DWORD unk0x44;			//0x44
	stat* pSetStat;			//0x48
	WORD wSetStatCount;		//0x4C
};

struct statlistex
{
	DWORD pMemPool;		//0x00
	DWORD unk0x04;		//0x04
	DWORD dwOwnerType;	//0x08
	DWORD dwOwnerId;	//0x0C
	DWORD dwListFlags;	//0x10
	DWORD unk0x14[4];	//0x14
	statex BaseStats;	//0x24
	statlist* pLastList;	//0x2C
	DWORD unk0x30;		//0x30
	statlistex* pListEx;	//0x34
	statlistex* pNextListEx;//0x38
	statlist* pMyLastList;	//0x3C
	statlist* pMyStats;	//0x40
	D2Unit* pOwner;		//0x44
	statex FullStats;	//0x48
	statex ModStats;	//0x50
	BYTE* StatFlags;	//0x58 ptr to an  array
	void* fpCallBack;	//0x5C
	D2Game* pGame;		//0x60
};

struct waypointdata
{
	WORD nFlags[12];						//0x00
};

struct inventory
{
	DWORD dwSignature;		//0x00
	void* pMemPool;			//0x04
	D2Unit* pOwner;			//0x08
	D2Unit* pFirstItem;		//0x0C
	D2Unit* pLastItem;		//0x10
	void* pInvInfo;			//0x14
	int nInvInfo;			//0x18
	DWORD dwLeftItemGUID;		//0x1C
	D2Unit* pCursorItem;		//0x20
	DWORD dwOwnerId;		//0x24
	DWORD dwItemCount;		//0x28
	DWORD unk0x2C[2];		//0x2C
	void* pFirstCorpse;		//0x34
	DWORD CorpseNumber;		//0x38
};

struct missiledata //sizeof 0x34
{
	DWORD                    _00;                   // +00 - some type of coords, see D2Common.11128, D2Common.11131 - D2Common.11134
	short                    nStreamMissile;        // +04
	short                    nStreamRange;          // +06
	short                    nActivateFrame;        // +08
	short                    nSkill;                // +0A
	short                    nLevel;                // +0C
	short                    nTotalFrames;          // +0E
	short                    nCurrentFrame;         // +10
	WORD                     _12;                   // +12
	DWORD                    fFlags;                // +14
	int                      nOwnerType;            // +18
	DWORD                    dwOwnerGUID;           // +1C
	int                      nHomeType;             // +20
	DWORD                    dwHomeGUID;            // +24
	int                      nStatus;               // +28 - used only by heatseeking projectiles
	pathpoint                pCoords;               // +2C - same, these are not coords, they are missile streams etc, see D2Common.11213 & D2Common.11214
	void*                    pStream;               // +30
};

struct itemdata
{
	DWORD dwQualityNo;		//0x00
	DWORD dwLowSeed;		//0x04
	DWORD dwHighSeed;		//0x08
	DWORD dwOwnerGUID;		//0x0C
	DWORD dwInitSeed;		//0x10
	DWORD dwCommandFlags;		//0x14
	DWORD dwItemFlags;		//0x18
	DWORD unk0x1C[2];		//0x1C
	DWORD dwActionStamp;		//0x24
	DWORD dwFileIndex;		//0x28
	DWORD dwItemLevel;		//0x2C
	WORD wItemFormat;		//0x30
	WORD wRarePrefix;		//0x32
	WORD wRareSuffix;		//0x34
	WORD wAutoPrefix;		//0x36
	WORD wMagicPrefix[3];		//0x38
	WORD wMagicSuffix[3];		//0x3E
	BYTE nBodyLoc;			//0x44
	BYTE nInvPage;			//0x45
	BYTE unk0x46[2];		//0x46
	BYTE nEarLvl;			//0x48
	BYTE nInvGfxIdx;		//0x49
	char szPlayerName[16];		//0x4A
	BYTE unk0x5A[2];		//0x5A
	inventory* pNodeInv;		//0x5C
	DWORD unk0x60;			//0x60
	D2Unit* pNextItem;		//0x64
	BYTE nNodePos;			//0x68
	BYTE nNodePosOther;		//0x69
};

struct client //size 0x518
{
	DWORD ClientID;                 //0x00 
	DWORD InitStatus;			   //0x04 Flag 0x4 - player is in game
	WORD ClassId;                   //0x08 Something with Arena, also could be equivalent of ClassId
	WORD PlayerStatus;			   //0x0A
	BYTE ClassId2;				   //0x0C
	char CharName[16];			   //0x0D 
	char AccountName[16];		   //0x1D 
	BYTE _3[50];                    //0x2D 
	DWORD _3b;					   //0x60
	DWORD _4;		               //0x64
	void* pGameData;				   //0x68
	DWORD _5[64];                   //0x6C 
	DWORD UnitType;				   //0x16C
	DWORD UnitId;                   //0x170
	D2Unit* pPlayerUnit;           //0x174
	DWORD _6;	                   //0x178 some bool
	void * ptSaveFile;			   //0x17C
	DWORD nOpenSize;				   //0x180
	DWORD _7[9];					   //0x184
	D2Game * pGame;					   //0x1A8 
	DWORD ActNo;					   //0x1AC
	DWORD _8;	                   //0x1B0
	void* ptRoom; //room1 * ptRoom;				   //0x1B4
	void* Packet[3]; //packetdata * Packet[3];		   //0x1B8
	DWORD _10[132];				   //0x1C4
	DWORD dwFlags;				   //0x3D4
	DWORD LastPacketTime;		   //0x3D8 GetTickCount()
	struct
	{
		WORD SkillId;
		WORD SkillUnk;
		DWORD SkillFlags;
	} ClientHotKeys[16];  		   //0x3DC 
	DWORD bWeaponSwitch;			   //0x45C
	char szClanTag[11];				   //0x460
	DWORD InactivityTime;		   //0x48C (seconds)
	WORD CurrentLife;			   //0x490
	WORD CurrentMana;			   //0x492
	WORD CurrentStamina;			   //0x494
	BYTE LifePotionPercent;		   //0x496
	BYTE ManaPotionPercent;		   //0x497
	WORD xPos;					   //0x498
	WORD yPos;					   //0x49A
	WORD xTargetOffset;			   //0x49C send by packets 0x96, 0x95, 0x18 (last arg) its converted to BYTE
	WORD yTargetOffset;			   //0x49E like above
	DWORD BodyGold;				   //0x4A0
	DWORD CurrentExp;			   //0x4A4
	client * ptPrevious;  	   //0x4A8 
	client * ptNextByID;		   //0x4AC 
	client * ptNextByName;       //0x4B0
	DWORD _12[19];				   //0x4B4
	DWORD SentPings;				   //0x500 Increasing every 10 secs
	DWORD bNeedToKnow;			   //0x504 u can set this true by packet 0x70
	DWORD ExpLoss;				   //0x508 its temp value, dont use
	DWORD LocaleID;				   //0x50C
	DWORD _13[2];                   //0x510 2 last elements are unused
};

struct playerdata
{
	char szName[16];			//0x00
	void* pQuestData[3]; //questdata* pQuestData[3];		//0x10
	waypointdata* pWaypointData[3];		//0x1C
	DWORD unk0x28[3];			//0x28
	void* pArenaUnit; // arenaunit* pArenaUnit;			//0x34
	DWORD unk0x38[4];			//0x38
	DWORD dwUniqueId;			//0x48
	DWORD dwTradeTick;			//0x4C
	DWORD dwTradeState;			//0x50
	DWORD unk0x54;				//0x54
	DWORD dwAcceptTradeTick;		//0x58
	void* pTrade; // playertrade* pTrade;			//0x5C
	DWORD unkx60[3];			//0x60
	DWORD dwBoughtItemId;			//0x6C
	DWORD dwRightSkill;			//0x70
	DWORD dwLeftSkill;			//0x74
	DWORD dwRSkillFlags;			//0x78
	DWORD dwLSkillFlags;			//0x7C
	DWORD dwSwitchRightSkill;		//0x80
	DWORD dwSwitchLeftSkill;		//0x84
	DWORD dwSwitchRSkillFlags;		//0x88
	DWORD dwSwitchLSkillFlags;		//0x8C
	DWORD unk0x90[3];			//0x90
	client* pClient;			//0x9C
	DWORD unk0xA0[48];			//0xA0
	DWORD dwHostileDelay;			//0x160
	DWORD unk0x164;				//0x164
	DWORD dwGameFrame;			//0x168
};

struct D2Unit;

struct path               // 0x1E0 bytes, I think
{
	DWORD PrecisionX;         // 0x000
	DWORD PrecisionY;         // 0x004
	DWORD targetX;            // 0x008
	DWORD targetY;            // 0x00C
	pathpoint sP;             // 0x010
	pathpoint sP2;            // 0x014
	pathpoint sP3;            // 0x018
	void* ptRoom;             // 0x01C
	void* ptRoom2;            // 0x020
	DWORD _1;                 // 0x024
	int nPathPoints;          // 0x028
	DWORD _01;                // 0x02C
	D2Unit* ptUnit;             // 0x030
	DWORD dwFlags;            // 0x034
	DWORD _2;                 // 0x038
	DWORD PathType;           // 0x03C
	DWORD PrevPathType;       // 0x040
	DWORD UnitSize;           // 0x044
	DWORD CollisionPattern;   // 0x048
	DWORD CollisionType;      // 0x04C
	DWORD _31;                // 0x050
	WORD _32;                 // 0x054
	WORD _33;                 // 0x056
	D2Unit* hTarget;            // 0x058
	int TargetType;           // 0x05C
	DWORD TargetId;           // 0x060 // not always target id. . .only if a target is specified
	BYTE bDirection;          // 0x064
	BYTE bNewDirection;       // 0x065
	BYTE bDiffDirection;      // 0x066
	BYTE _4;                  // 0x067
	BYTE _5[20];              // 0x068 // vector stuff, maybe?, with some collision stuff, too?
	DWORD Velocity;           // 0x07C // Velocity of the unit 
	DWORD _6;                 // 0x080 // unknown
	DWORD Velocity2;          // 0x084 // seems to be a repeat of 0x07C
	DWORD _8[2];              // 0x088 // Unknown what these are.
	BYTE bDist;               // 0x090
	BYTE bDistMax;            // 0x091
	BYTE _9;                  // 0x092
	BYTE bStepNum;            // 0x093
	int nDist;                // 0x094
	DWORD PathOffset;         // 0x098
	pathpoint PathPoints[7];  // 0x09C // Pathing points.
	DWORD _11[70];            // 0x0BC // All these have been nulls, so far
	bool _12;                 // 0x1D4 // I'm assuming this is bool, because its always either 1 or 0
	pathpoint _13;            // 0x1D8 // Values always seem to be slightly off from the ones above.
	// As far as I can tell, this is the end of the struct, but I'll pad it to fill it to an even val, like
	// most structs are.
	DWORD Pad;                // 0x1DC
};


struct D2Unit
{
	DWORD  dwUnitType;					//0x00

	union							//0x04
	{
		DWORD dwLineId;					// the id in the txt file
		DWORD dwClassId;
	};

	void* pMemoryPool;					//0x08
	DWORD dwUnitId;						//0x0C
	DWORD dwAnimMode;					//0x10
	union							//0x14
	{
		playerdata* pPlayerData;
		itemdata* pItemData;
		void* pMonsterData; //monsterdata* pMonsterData;
		void* pObjectData; //  objectdata* pObjectData;
		missiledata* pMissileData;
	};
	DWORD dwAct;						//0x18
	void* pDrlgAct; //drlgact* pDrlgAct;				        //0x1C
	DWORD dwLoSeed;						//0x20
	DWORD dwHiSeed;						//0x24
	DWORD dwInitSeed;					//0x28

	union							//0x2C
	{
		void* pDynamicPath; // dynamicpath* pDynamicPath;
		path* pStaticPath; // staticpath* pStaticPath;
	};

	void* pAnimSeq; //animseq* pAnimSeq;				        //0x30
	DWORD dwSeqFrameCount;					//0x34
	DWORD dwSeqFrame;					//0x38
	DWORD dwAnimSpeed;					//0x3C
	DWORD dwSeqMode;					//0x40
	DWORD dwGFXcurrentFrame;				//0x44
	DWORD dwFrameCount;					//0x48
	WORD wAnimSpeed;					//0x4C
	WORD wActionFrame;					//0x4E
	void* pAnimData;  // animdata* pAnimData;				        //0x50
	void* pGfxData; //gfxdata* pGfxData;				        //0x54
	void* pGfxData2; //  gfxdata* pGfxDataCopy;				        //0x58
	statlistex* pStatListEx;				//0x5C
	inventory* pInventory;				        //0x60

	union							//0x64
	{
		DWORD dwInteractGUID;		                //Server pUnit
		void* pLight; // light* pLight;		                        //Client pUnit
	};

	union							//0x68
	{
		DWORD dwInteractType;		                //Server pUnit
		DWORD dwStartLight;		                //Client pUnit
	};

	union							//0x6C
	{
		WORD wInteract;			                //Server Punit
		WORD wPL2ShiftIndex;		                //Client pUnit
	};

	WORD wUpdateType;					//0x6E
	D2Unit* pUpdateUnit;				        //0x70
	void* pQuestRecord;	//questrecord* pQuestRecord;			        //0x74
	BOOL bSparkChest;					//0x78
	void* pTimerArg; //timerarg* pTimerArg;				        //0x7C

	union							//0x80
	{
		D2Game* pGame;		                        //Server pUnit
		DWORD dwSoundSync;		                //Client pUnit
	};

	WORD wXpos;						//0x84
	WORD wYpos;						//0x86
	WORD unk0x88[4];					//0x88
	void* pEventList; //eventlist* pEventList;				        //0x90
	DWORD dwOwnerType;					//0x94
	DWORD dwOwnerGUID;					//0x98
	DWORD unk0x9C[2];					//0x9C
	void* pHoverText; //hovertext* pHoverText;				        //0xA4
	void* pSkills; // skillist* pSkills;				        //0xA8
	void* pCombat; //combat* pCombat;					//0xAC
	DWORD dwLastHitClass;					//0xB0
	DWORD unk0xB4;						//0xB4
	DWORD dwDropItemCode;					//0xB8
	DWORD unk0xBC[2];					//0xBC
	DWORD dwFlags;						//0xC4
	DWORD dwFlagEx;						//0xC8

	union							//0xCC
	{
		void* pSrvQuestData; //questsrv* pSrvQuestData;	                //Server pUnit
		void* pCltQuestData; //questclt* pCltQuestData;	                //Client pUnit
	};

	DWORD dwNodeIndex; //0xD0
	DWORD dwTickCount;					//0xD4

	union							//0xD8
	{
		DWORD dwSrvTickCount;		                //Server pUnit
		void* pParticleStream; //particlestream* pParticleStream;	        //Client pUnit	
	};

	void* pTimer; // timer* pTimer;					        //0xDC
	D2Unit* pChangeNextUnit;				        //0xE0
	D2Unit* pRoomNext;					//0xE4
	D2Unit* pListNext;					//0xE8
	void* pMsgFirst;					//0xEC
	void* pMsgLast;						//0xF0
};

struct D2Game
{
	DWORD _1[3];				//0x00
	DWORD * ptGameData8;		//0x0C
	D2Game* pNext;				//0x10
	DWORD _1a;				//0x14
	CRITICAL_SECTION* ptLock; //0x18
	void * pMemPool;			//0x1C - not used, always NULL
	void * pGameData;			//0x20
	DWORD _2;					//0x24
	WORD  wServerToken;		//0x28 called 'Server Ticket' aswell
	char szGameName[16];		//0x2A
	char szGamePass[16];		//0x3A
	char szGameDesc[32];		//0x4A
	BYTE bGameType;			//0x6A - whenever this is single player (etc)
	BYTE _3a[2];				//0x6B _3a[0] - Arena's _2;
	BYTE nDifficulty;		//0x6D
	BYTE _4[2];				//0x6E
	DWORD LODFlag;			//0x70
	DWORD dwGameType;			//0x74
	WORD  wItemFormat;         //0x78
	WORD  _5;					//0x7A
	DWORD InitSeed;			//0x7C
	DWORD ObjSeed;            //0x80 - seed used for object spawning
	DWORD InitSeed2;          //0x84 (another instance, dunno why)
	client * pClient;  //0x88 - (pClient structure of last player that entered the game)
	DWORD nClients;			//0x8C
	DWORD nUnits[6];          //0x90 - array of 6 counters, one for each unit type, this is the next GUID used too
	DWORD GameFrame;          //0xA8 - the current frame of the game, used for timers (buff duration etc)
	DWORD _6[3];				//0xAC
	void* pTimerQueue; //timerqueue * pTimerQueue;       //0xB8 a queue of all current active and inactive timers
	void* pDrlgAct[5]; //drlgact * pDrlgAct[5];			//0xBC
	DWORD GameSeed[2];			//0xD0
	void* pDrlgRoomList[5];	//0xD8
	DWORD dwMonSeed;				//0xEC - seed used for monster spawning
	DWORD* pMonsterRegion[1024];  //0xF0 - one pointer for each of the 1024 possible levels
	DWORD* pObjectControl;		//0x10F0 - a controller holding all object region structs
	void* pQuestControl; //questcontrol * pQuestControl;	//0x10F4 - a controller holding all quest info
	void* pUnitNodes[10]; //unitnode * pUnitNodes[10];		//0x10F8 - ten lists of unit node lists, this is used by the AI target seeking code (and other stuff)
	D2Unit* pUnitList[5][128];	//0x1120 - 5 lists of 128 lists of units (see pUnit documentation), second index is GUID & 127, BEWARE: since ever, missiles are array #4 and items are array #3 (so type3=index4 and type4=index3)
	DWORD* pTileList;			    //0x1B20 - a list for all VisTile units
	DWORD dwUniqueFlags[128];		//0x1B24 - 128 DWORDS worth of flags that control if a unique item got spawned [room for 4096]
	void* pNpcControl; //npccontrol * pNpcControl;		//0x1D24 - a controller holding all npc info (like store inventories, merc list)
	void* pArenaControl; //arenacontrol * pArenaControl;			//0x1D28 - a controller for arena stuff, functional and also used in game init
	void* pPartyControl; //partycontrol* pPartyControl;			//0x1D2C - a controller for all party related stuff
	BYTE nBossFlagList[64];			//0x1D30 - 64 bytes for handling 512 super unique monsters (if they were spawned etc)
	DWORD dwMonModeData[17];		//0x1D70 - related to monsters changing mode
	DWORD nMonModeData;			//0x1DB4 - counter related to the above
	DWORD _7;						//0x1DB8
	DWORD nCreateTick;			//0x1DBC
	DWORD _8;						//0x1DC0
	DWORD nSyncTimer;				//0x1DC4 - used to sync events
	DWORD _9[8];					//0x1DC8
};

#pragma pack()



#endif