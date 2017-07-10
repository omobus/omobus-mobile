/* -*- C++ -*- */
/* This file is a part of the omobus-mobile project.
 * Copyright (c) 2006 - 2013 ak-obs, Ltd. <info@omobus.net>.
 * Author: Igor Artemov <i_artemov@ak-obs.ru>.
 *
 * This program is commerce software; you can't redistribute it and/or
 * modify it.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <windows.h>
#include <omobus-mobile.h>

static const FILETIME	ftJan1970 = {3577643008,27111902};

// Compare two 64-bit values.
// Returns:
//   <0 if a is less than b
//   0  if a is equal to b
//   >0 if a is greater than b
static 
int cmp64(const ULARGE_INTEGER* a, const ULARGE_INTEGER* b)
{
	if (a == NULL || b == NULL)
		return -1;	// error but no error return value

	if (a->HighPart < b->HighPart)
		return -1;
	else if (a->HighPart == b->HighPart)
	{
		if (a->LowPart < b->LowPart)
			return -1;
		else if (a->LowPart == b->LowPart)
			return 0;
		else	// if (a->LowPart > b->LowPart)
			return 1;
	}
	else	// if (a->HighPart < b->HighPart)
		return 1;
}


// Returns position of top non-zero bit,
// eg. -1 = none set, 0 = first bit, 31 = top bit
static 
int topBit(DWORD value)
{
	if (value == 0)
		return -1;
	else if (value & 0xffff0000)
	{	// bit in 0xffff0000 is set
		if (value & 0xff000000)
		{	// bit in 0xff000000 is set
			if (value & 0xf0000000)
			{	// bit in 0xf0000000 is set
				if (value & 0xc0000000)
				{	// bit in 0xc0000000 is set
					if (value & 0x80000000)
						return 31;
					else
						return 30;
				}
				else
				{	// bit in 0x30000000 is set
					if (value & 0x20000000)
						return 29;
					else
						return 28;
				}
			}
			else
			{	// bit in 0x0f000000 is set
				if (value & 0x0c000000)
				{	// bit in 0x0c000000 is set
					if (value & 0x08000000)
						return 27;
					else
						return 26;
				}
				else
				{	// bit in 0x03000000 is set
					if (value & 0x02000000)
						return 25;
					else
						return 24;
				}
			}
		}
		else
		{	// bit in 0x00ff0000 is set
			if (value & 0x00f00000)
			{	// bit in 0x00f00000 is set
				if (value & 0x00c00000)
				{	// bit in 0x00c00000 is set
					if (value & 0x00800000)
						return 23;
					else
						return 22;
				}
				else
				{	// bit in 0x00300000 is set
					if (value & 0x00200000)
						return 21;
					else
						return 20;
				}
			}
			else
			{	// bit in 0x000f0000 is set
				if (value & 0x000c0000)
				{	// bit in 0x000c0000 is set
					if (value & 0x00080000)
						return 19;
					else
						return 18;
				}
				else
				{	// bit in 0x00030000 is set
					if (value & 0x00020000)
						return 17;
					else
						return 16;
				}
			}
		}
	}
	else
	{	// bit in 0x0000ffff is set
		if (value & 0x0000ff00)
		{	// bit in 0x0000ff00 is set
			if (value & 0x0000f000)
			{	// bit in 0x0000f000 is set
				if (value & 0x0000c000)
				{	// bit in 0x0000c000 is set
					if (value & 0x00008000)
						return 15;
					else
						return 14;
				}
				else
				{	// bit in 0x00003000 is set
					if (value & 0x00002000)
						return 13;
					else
						return 12;
				}
			}
			else
			{	// bit in 0x00000f00 is set
				if (value & 0x00000c00)
				{	// bit in 0x00000c00 is set
					if (value & 0x00000800)
						return 11;
					else
						return 10;
				}
				else
				{	// bit in 0x00000300 is set
					if (value & 0x00000200)
						return 9;
					else
						return 8;
				}
			}
		}
		else
		{	// bit in 0x000000ff is set
			if (value & 0x000000f0)
			{	// bit in 0x000000f0 is set
				if (value & 0x000000c0)
				{	// bit in 0x000000c0 is set
					if (value & 0x00000080)
						return 7;
					else
						return 6;
				}
				else
				{	// bit in 0x00000030 is set
					if (value & 0x00000020)
						return 5;
					else
						return 4;
				}
			}
			else
			{	// bit in 0x0000000f is set
				if (value & 0x0000000c)
				{	// bit in 0x0000000c is set
					if (value & 0x00000008)
						return 3;
					else
						return 2;
				}
				else
				{	// bit in 0x00000003 is set
					if (value & 0x00000002)
						return 1;
					else
						return 0;
				}
			}
		}
	}
}


// Returns position of top non-zero bit,
// eg. -1 = none set, 0 = first bit, 63 = top bit
static 
int topBit64(ULARGE_INTEGER* value)
{
	int		result;

	if (value == NULL)
		return 0;

	result = topBit(value->HighPart);
	if (result != -1)
		return result+32;
	else
		return topBit(value->LowPart);
}


static 
void shl64(ULARGE_INTEGER* value, int shift)
{
	ULARGE_INTEGER	result;

	if (value == NULL || shift <= 0)
		return;

	result.HighPart = (value->HighPart << shift) | (value->LowPart >> (32-shift));
	result.LowPart = (value->LowPart << shift);

	value->HighPart = result.HighPart;
	value->LowPart = result.LowPart;
}


static 
void shr64(ULARGE_INTEGER* value, int shift)
{
	ULARGE_INTEGER	result;

	if (value == NULL || shift <= 0)
		return;

	result.HighPart = (value->HighPart >> shift);
	result.LowPart = (value->LowPart >> shift) | (value->HighPart << (32-shift));

	value->HighPart = result.HighPart;
	value->LowPart = result.LowPart;
}


// Add valueToAdd to value (doesn't handle overflow)
static 
void add64(ULARGE_INTEGER* value, ULARGE_INTEGER* valueToAdd)
{
	if (value == NULL || valueToAdd == NULL)
		return;

	value->LowPart += valueToAdd->LowPart;
	if (value->LowPart < valueToAdd->LowPart)
		value->HighPart++;	// carry to HighPart
	value->HighPart += valueToAdd->HighPart;
}


// Subtract valueToSubtract from value (doesn't handle underflow)
static 
void sub64(ULARGE_INTEGER* value, ULARGE_INTEGER* valueToSubtract)
{
	if (value == NULL || valueToSubtract == NULL)
		return;

	if (value->LowPart < valueToSubtract->LowPart)
		value->HighPart--;	// borrow from HighPart
	value->HighPart -= valueToSubtract->HighPart;
	value->LowPart -= valueToSubtract->LowPart;
}


static 
void mul64(ULARGE_INTEGER* value, DWORD multiplier)
{
	ULARGE_INTEGER	result = { 0, 0 };
	ULARGE_INTEGER	temp;
	WORD			multiplierHigh = (WORD)(multiplier >> 16);
	WORD			multiplierLow = (WORD)(multiplier & 0xffff);

	if (value == NULL)
		return;

	//
	// split the DWORD in two, and multiply seperately to avoid overflow
	//

	// firstly, the lower 16-bits of the multiplier

	temp.HighPart = 0;
	temp.LowPart = (value->LowPart & 0xffff) * multiplierLow;
	add64(&result, &temp);

	temp.LowPart = (value->LowPart >> 16) * multiplierLow;
	shl64(&temp, 16);
	add64(&result, &temp);

	temp.LowPart = 0;
	temp.HighPart = (value->HighPart & 0xffff) * multiplierLow;
	add64(&result, &temp);

	temp.HighPart = (value->HighPart >> 16) * multiplierLow;
	shl64(&temp, 16);
	add64(&result, &temp);

	// secondly, the higher 16-bits of the multiplier

	temp.HighPart = 0;
	temp.LowPart = (value->LowPart & 0xffff) * multiplierHigh;
	shl64(&temp, 16);
	add64(&result, &temp);

	temp.LowPart = (value->LowPart >> 16) * multiplierHigh;
	shl64(&temp, 32);
	add64(&result, &temp);

	temp.LowPart = 0;
	temp.HighPart = (value->HighPart & 0xffff) * multiplierHigh;
	shl64(&temp, 16);
	add64(&result, &temp);

	temp.HighPart = (value->HighPart >> 16) * multiplierHigh;
	shl64(&temp, 32);
	add64(&result, &temp);

	// return the result
	value->HighPart = result.HighPart;
	value->LowPart = result.LowPart;
}

static 
void div64(ULARGE_INTEGER* value, DWORD divisor)
{
	ULARGE_INTEGER	result = { 0, 0 };
	ULARGE_INTEGER	shiftedDivisor;	// divisor shifted to left
	ULARGE_INTEGER	shiftedOne;		// '1' shifted to left by same number of bits as divisor
	int				shift;

	if (value == NULL)
		return;
	if (divisor == 0)
	{
		value->LowPart = 0;
		value->HighPart = 0;
		return;
	}
	if (value->HighPart == 0)
	{
		if (value->LowPart != 0)
			value->LowPart /= divisor;
		return;
	}

	// shift divisor up (into shifted) as far as it can go before it is greater than value
	shift = topBit64(value) - topBit(divisor);
	shiftedDivisor.LowPart = divisor;
	shiftedDivisor.HighPart = 0;
	shiftedOne.LowPart = 1;
	shiftedOne.HighPart = 0;
	shl64(&shiftedDivisor, shift);
	shl64(&shiftedOne, shift);
	while (shift >= 0)
	{
		if (cmp64(&shiftedDivisor, value) <= 0)
		{
			add64(&result, &shiftedOne);
			sub64(value, &shiftedDivisor);
		}
		shr64(&shiftedDivisor, 1);
		shr64(&shiftedOne, 1);
		shift--;
	}

	value->HighPart = result.HighPart;
	value->LowPart = result.LowPart;
}

// -----------------------------------------------------------------------------
// Convert Win32 FILETIME into time_t
static
time_t w32_filetime_to_time_t(FILETIME* ft)
{
	// make sure ft is at least ftJan1970
	if (cmp64((ULARGE_INTEGER*)ft, (ULARGE_INTEGER*)&ftJan1970) < 0)
		return -1;

	// subtract ftJan1970 from ft
	sub64((ULARGE_INTEGER*)ft, (ULARGE_INTEGER*)&ftJan1970);

	// divide ft by 10,000,000 to convert from 100-nanosecond units to seconds
	div64((ULARGE_INTEGER*)ft, 10000000);

	// bound check result
	if (ft->dwHighDateTime != 0 || ft->dwLowDateTime >= 2147483648)
		return -1;		// value is too big to return in time_t

	return (time_t)ft->dwLowDateTime;
}

// -----------------------------------------------------------------------------
// Аналог ANSI функции time
time_t time(time_t* t)
{
	SYSTEMTIME		stNow;
	FILETIME		ftNow;
	time_t			tt;

	// get system time
	GetSystemTime(&stNow);
	if (!SystemTimeToFileTime(&stNow, &ftNow))
		return -1;

	tt = w32_filetime_to_time_t(&ftNow);

	if (t != NULL)
		*t = tt;
	return tt;
}

time_t omobus_time()
{
  return time(NULL);
}

// -----------------------------------------------------------------------------
