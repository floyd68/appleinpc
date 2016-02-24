// Slots.cpp: implementation of the CSlots class.
//
//////////////////////////////////////////////////////////////////////

#include "arch/frame/stdafx.h"
#include "arch/frame/mainfrm.h"

#include "slots.h"
#include "appleclock.h"
#include "phasor.h"
#include "diskinterface.h"
#include "mousecard.h"
#include "sddiskii.h"

extern BYTE MemReturnRandomData(BYTE highbit);
extern CALLBACK_HANDLER(OnDiskLightChange);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
IMPLEMENT_SERIAL( CSlots, CObject, 1 );

CSlots::CSlots()
{
	int i;
	for( i = 0; i < MAX_SLOTS; i++ )
	{
		m_slots[i] = NULL;
	}
	m_nDiskSlotNum = -1;
	m_nHardDiskSlotNum = -1;
}

CSlots::~CSlots()
{
	int i;
	for( i=0; i < MAX_SLOTS; i++ )
	{
		if ( m_slots[i] != NULL )
		{
			delete m_slots[i];
			m_slots[i] = NULL;
		}
	}
}

// slot�� ī�尡 ����� ��� ���������� ������ �ϵ��� �Ѵ�.
BOOL CSlots::Initialize()
{
	int i, nDeviceNum;
	int nDiskInterface;
	nDiskInterface = 0;
	m_nDiskSlotNum = -1;
	m_nHardDiskSlotNum = -1;

	for ( i = MAX_SLOTS-1; i >= 0; i-- )
	{
		if ( m_slots[i] != NULL )
		{
			nDeviceNum = m_slots[i]->GetDeviceNum();
			switch( nDeviceNum )
			{
			case CARD_DISK_INTERFACE:
				((CDiskInterface*)m_slots[i])->SetMotorLightHandler( (void*)nDiskInterface, OnDiskLightChange );
				nDiskInterface ^= 1;
				if ( m_nDiskSlotNum == -1 )
				{
					m_nDiskSlotNum = i;
				}
				break;
			case CARD_SD_DISK_II:
				((CSDDiskII*)m_slots[i])->SetMotorLightHandler( (void*)nDiskInterface, OnDiskLightChange );
				nDiskInterface ^= 1;
				if ( m_nHardDiskSlotNum == -1 )
				{
					m_nHardDiskSlotNum = i;
				}
				break;
			case CARD_MOUSE_INTERFACE:
			case CARD_PHASOR:
				break;
			}
		}
	}
	return TRUE;
}

void CSlots::ConfigureDiskette()
{
	if ( m_nDiskSlotNum >= 0 )
	{
		((CDiskInterface*)m_slots[m_nDiskSlotNum])->Configure();
	}
}

void CSlots::ConfigureHardDisk()
{
	if ( m_nHardDiskSlotNum >= 0 )
	{
		((CSDDiskII*)m_slots[m_nHardDiskSlotNum])->Configure();
	}
}

BOOL CSlots::HasDiskInterface()
{
	return ( m_nDiskSlotNum >= 0 );
}

BOOL CSlots::HasHardDiskInterface()
{
	return ( m_nHardDiskSlotNum >= 0 );
}


void CSlots::PowerOn()
{
	int i;
	for( i = 0; i < MAX_SLOTS; i++ )
		if ( m_slots[i] != NULL )
			m_slots[i]->PowerOn();
}

void CSlots::PowerOff()
{
	int i;
	for( i = 0; i < 7; i++ )
		if ( m_slots[i] != NULL )
			m_slots[i]->PowerOff();
}

BYTE CSlots::Read(WORD addr)
{
	int slot;
	BYTE retval;
	slot = ( ( addr >> 4 ) & 0x07 ) - 1;
	if ( m_slots[slot] )
	{
		retval = m_slots[slot]->Read(addr);
		return( retval );
	}
	return MemReturnRandomData( 0 );
}

BYTE CSlots::ReadRom(WORD addr)
{
	int slot;
	slot = ( ( addr >> 8 ) & 0x7 ) - 1;
	if( m_slots[slot] )
		return m_slots[slot]->ReadRom(addr);
				
	return MemReturnRandomData( 0 );
}

void CSlots::Write(WORD addr, BYTE data)
{
	int slot;
	slot = ( ( addr >> 4 ) & 0x07 ) - 1;
	if ( m_slots[slot] )
		m_slots[slot]->Write(addr, data);
}

void CSlots::WriteRom(WORD addr, BYTE data)
{
	int slot;
	slot = ( ( addr >> 8 ) & 0x7 ) - 1;
	if ( m_slots[slot] )
		m_slots[slot]->WriteRom(addr, data);
}

void CSlots::Reset()
{
	int i;
	for( i = 0; i < MAX_SLOTS; i++ )
	{
		if ( m_slots[i] != NULL )
		{
			m_slots[i]->Reset();
		}
	}
}

void CSlots::Clock(DWORD clock)
{
	int i;
	for ( i = 0; i < MAX_SLOTS; i++ )
	{
		if ( m_slots[i] )
			m_slots[i]->Clock( clock );
	}
}

CCard* CSlots::GetCard(int slot)
{
	if ( slot < 0 || slot >= MAX_SLOTS )
		return NULL;
	return m_slots[slot];
}

void CSlots::RemoveCard(int nSlot)
{
	if ( m_slots[nSlot] == NULL )
		return;
	delete m_slots[nSlot];
	m_slots[nSlot] = NULL;
}

BOOL CSlots::InsertCard(int nSlot, int nDeviceNum)
{
	CCard* pCard;
	if ( m_slots[nSlot] != NULL )
		return FALSE;

	switch( nDeviceNum & 0xFF )
	{
	case CARD_DISK_INTERFACE:
		pCard = new CDiskInterface();
		break;
	case CARD_MOUSE_INTERFACE:
		pCard = new CMouseCard();
		break;
	case CARD_PHASOR:
		pCard = new CPhasor();
		break;
	case CARD_SD_DISK_II:
		pCard = new CSDDiskII();
		break;
	default:
		return FALSE;
	}
	m_slots[nSlot] = pCard;
	pCard->SetDipSwitch(nDeviceNum >> 8);
	return TRUE;
}

BOOL CSlots::InsertCard(int nSlot, CCard* pCard)
{
	if ( m_slots[nSlot] != NULL || pCard == NULL )
		return FALSE;
	m_slots[nSlot] = pCard;
	return TRUE;
}

void CSlots::SetDipSwitch(int nSlot, int nDipSwitch)
{
	if ( nSlot < 0 || nSlot >	MAX_SLOTS )
		return;
	if ( m_slots[nSlot] == NULL )
		return;
	m_slots[nSlot]->SetDipSwitch(nDipSwitch);
}

void CSlots::Serialize(CArchive &ar)
{
	CObject::Serialize( ar );
	int i;
	int nDeviceNum;

	if ( ar.IsStoring() )
	{
		for( i = 0; i < MAX_SLOTS; i++ )
		{
			if ( m_slots[i] == NULL )
			{
				ar << CARD_EMPTY;
			}
			else
			{
				ar << m_slots[i]->GetDeviceNum();
				m_slots[i]->Serialize(ar);
			}
		}
	}
	else
	{
		for( i = 0; i < MAX_SLOTS; i++ )
		{
			RemoveCard( i );
			ar >> nDeviceNum;
			if ( nDeviceNum != CARD_EMPTY )
			{
				if ( InsertCard( i, nDeviceNum ) == TRUE )
				{
					m_slots[i]->Serialize(ar);
				}
			}
		}
		Initialize();
	}
}