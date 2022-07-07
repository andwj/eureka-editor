//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022 Ioan Chera
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "m_select.h"
#include "gtest/gtest.h"

TEST(MSelect, ChangeType)
{
	selection_c selection(ObjType::things);
	ASSERT_EQ(selection.what_type(), ObjType::things);

	selection.change_type(ObjType::sectors);
	ASSERT_EQ(selection.what_type(), ObjType::sectors);
}

TEST(MSelect, InitiallyEmpty)
{
	selection_c selection;
	ASSERT_TRUE(selection.empty());
	ASSERT_FALSE(selection.notempty());
	ASSERT_EQ(selection.count_obj(), 0);
	ASSERT_EQ(selection.max_obj(), -1);
	ASSERT_EQ(selection.find_first(), -1);
	ASSERT_EQ(selection.find_second(), -1);
}

TEST(MSelect, CountObj)
{
	selection_c selection(ObjType::things);
	selection.set(2);
	selection.set(3);
	selection.set(5);
	ASSERT_EQ(selection.count_obj(), 3);
	ASSERT_FALSE(selection.empty());
	ASSERT_TRUE(selection.notempty());
}

TEST(MSelect, ChangingTypeClearsContent)
{
	selection_c selection(ObjType::things);
	selection.set(2);
	selection.set(3);
	selection.set(5);
	selection.change_type(ObjType::vertices);
	ASSERT_EQ(selection.count_obj(), 0);
	ASSERT_TRUE(selection.empty());
	ASSERT_FALSE(selection.notempty());
}

TEST(MSelect, ClearAll)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);
	selection.clear_all();
	ASSERT_EQ(selection.count_obj(), 0);
	ASSERT_TRUE(selection.empty());
	ASSERT_FALSE(selection.notempty());
	selection.set(4);
	ASSERT_EQ(selection.count_obj(), 1);
	ASSERT_FALSE(selection.empty());
	ASSERT_TRUE(selection.notempty());
}

TEST(MSelect, MaxObj)
{
	selection_c selection;
	selection.set(2);
	selection.set(5);
	selection.set(3);
	ASSERT_EQ(selection.max_obj(), 5);
}

TEST(MSelect, Get)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);
	ASSERT_FALSE(selection.get(0));
	ASSERT_FALSE(selection.get(1));
	ASSERT_TRUE(selection.get(2));
	ASSERT_TRUE(selection.get(3));
	ASSERT_FALSE(selection.get(4));
	ASSERT_TRUE(selection.get(5));
	ASSERT_FALSE(selection.get(6));	// going right is fine
}

TEST(MSelect, ClearOne)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);
	selection.clear(3);
	selection.clear(4);	// clearing unset is fine
	ASSERT_FALSE(selection.get(0));
	ASSERT_FALSE(selection.get(1));
	ASSERT_TRUE(selection.get(2));
	ASSERT_FALSE(selection.get(3));
	ASSERT_FALSE(selection.get(4));
	ASSERT_TRUE(selection.get(5));
}

TEST(MSelect, MaxObjGetsUpdated)
{
	selection_c selection;
	selection.set(2);
	selection.set(5);
	selection.set(3);
	selection.clear(5);
	ASSERT_EQ(selection.max_obj(), 3);
	selection.set(7);
	ASSERT_EQ(selection.max_obj(), 7);
}

TEST(MSelect, Toggle)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);
	selection.toggle(3);
	selection.toggle(4);	// clearing unset is fine
	ASSERT_FALSE(selection.get(0));
	ASSERT_FALSE(selection.get(1));
	ASSERT_TRUE(selection.get(2));
	ASSERT_FALSE(selection.get(3));
	ASSERT_TRUE(selection.get(4));
	ASSERT_TRUE(selection.get(5));
}

TEST(MSelect, GetExtOnNormalListReturnsFullMask)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);
	ASSERT_FALSE(selection.get_ext(0));
	ASSERT_FALSE(selection.get_ext(1));
	ASSERT_EQ(selection.get_ext(2), 255);
	ASSERT_EQ(selection.get_ext(3), 255);
	ASSERT_FALSE(selection.get_ext(4));
	ASSERT_EQ(selection.get_ext(5), 255);
	ASSERT_FALSE(selection.get_ext(6));	// going right is fine
}

TEST(MSelect, ExtendedList)
{
	selection_c selection(ObjType::things, true);
	selection.set_ext(2, 12);
	selection.set_ext(3, 23);
	selection.set_ext(5, 222);

	ASSERT_FALSE(selection.get_ext(0));
	ASSERT_FALSE(selection.get_ext(1));
	ASSERT_EQ(selection.get_ext(2), 12);
	ASSERT_EQ(selection.get_ext(3), 23);
	ASSERT_FALSE(selection.get_ext(4));
	ASSERT_EQ(selection.get_ext(5), 222);
	ASSERT_FALSE(selection.get_ext(6));	// going right is fine

	// Replacing value is fine
	selection.set_ext(3, 40);
	ASSERT_EQ(selection.get_ext(3), 40);
}

TEST(MSelect, SimpleSettingOnExtendedList)
{
	selection_c selection(ObjType::things, true);
	selection.set(2);
	selection.set(3);
	selection.set(5);

	ASSERT_FALSE(selection.get_ext(0));
	ASSERT_FALSE(selection.get_ext(1));
	ASSERT_EQ(selection.get_ext(2), 1);
	ASSERT_EQ(selection.get_ext(3), 1);
	ASSERT_FALSE(selection.get_ext(4));
	ASSERT_EQ(selection.get_ext(5), 1);
	ASSERT_FALSE(selection.get_ext(6));	// going right is fine
}

TEST(MSelect, SimpleGettingOnExtendedList)
{
	selection_c selection(ObjType::things, true);
	selection.set_ext(2, 12);
	selection.set_ext(3, 23);
	selection.set_ext(5, 222);

	ASSERT_FALSE(selection.get(0));
	ASSERT_FALSE(selection.get(1));
	ASSERT_TRUE(selection.get(2));
	ASSERT_TRUE(selection.get(3));
	ASSERT_FALSE(selection.get(4));
	ASSERT_TRUE(selection.get(5));
	ASSERT_FALSE(selection.get(6));	// going right is fine

	// Replacing value is fine
	selection.set_ext(3, 40);
	ASSERT_TRUE(selection.get(3));

	// Zeroing out value will clear it
	selection.set_ext(5, 0);
	ASSERT_FALSE(selection.get(5));
}

TEST(MSelect, CountExtendedList)
{
	selection_c selection(ObjType::things, true);
	selection.set_ext(2, 12);
	selection.set_ext(3, 23);
	selection.set_ext(5, 222);
	ASSERT_EQ(selection.count_obj(), 3);
	// Zeroing out value will clear it
	selection.set_ext(5, 0);
	ASSERT_EQ(selection.count_obj(), 2);

	// Also accept clearing
	selection.clear(2);
	ASSERT_EQ(selection.count_obj(), 1);
}

TEST(MSelect, CheckExtendedListEmpty)
{
	selection_c selection(ObjType::things, true);
	selection.set_ext(2, 12);
	selection.set_ext(3, 23);
	selection.set_ext(5, 222);
	ASSERT_FALSE(selection.empty());
	ASSERT_TRUE(selection.notempty());

	selection.clear(2);
	selection.set_ext(3, 0);
	ASSERT_FALSE(selection.empty());
	ASSERT_TRUE(selection.notempty());

	selection.clear(5);
	ASSERT_TRUE(selection.empty());
	ASSERT_FALSE(selection.notempty());
}

TEST(MSelect, CheckExtendedListEmptyAfterClearingAll)
{
	selection_c selection(ObjType::things, true);
	selection.set_ext(2, 12);
	selection.set_ext(3, 23);
	selection.set_ext(5, 222);
	ASSERT_EQ(selection.count_obj(), 3);
	ASSERT_FALSE(selection.empty());
	ASSERT_TRUE(selection.notempty());

	selection.clear_all();
	ASSERT_EQ(selection.count_obj(), 0);
	ASSERT_TRUE(selection.empty());
	ASSERT_FALSE(selection.notempty());
}

TEST(MSelect, MaxObjOnExtendedList)
{
	selection_c selection(ObjType::things, true);
	ASSERT_EQ(selection.max_obj(), -1);
	selection.set_ext(2, 12);
	ASSERT_EQ(selection.max_obj(), 2);
	selection.set_ext(3, 23);
	ASSERT_EQ(selection.max_obj(), 3);
	selection.set_ext(5, 222);
	ASSERT_EQ(selection.max_obj(), 5);

	selection.clear(3);
	ASSERT_EQ(selection.max_obj(), 5);
	selection.clear(5);
	ASSERT_EQ(selection.max_obj(), 2);
	selection.set_ext(3, 34);
	selection.set_ext(4, 222);
	ASSERT_EQ(selection.max_obj(), 4);
	selection.clear_all();
	ASSERT_EQ(selection.max_obj(), -1);
}

TEST(MSelect, CheckExtendedListClearedAfterChangingType)
{
	selection_c selection(ObjType::things, true);
	selection.set_ext(2, 12);
	selection.set_ext(3, 23);
	selection.set_ext(5, 222);
	selection.change_type(ObjType::linedefs);
	ASSERT_EQ(selection.count_obj(), 0);
	ASSERT_TRUE(selection.empty());
	ASSERT_FALSE(selection.notempty());
}

TEST(MSelect, Frob)
{
	selection_c selection;
	selection.frob(2, BitOp::add);
	selection.frob(3, BitOp::add);
	selection.frob(5, BitOp::add);
	ASSERT_EQ(selection.count_obj(), 3);
	ASSERT_EQ(selection.max_obj(), 5);
	ASSERT_TRUE(selection.get(2));
	ASSERT_TRUE(selection.get(3));

	selection.frob(4, BitOp::remove);
	ASSERT_EQ(selection.count_obj(), 3);
	ASSERT_TRUE(selection.get(2));
	ASSERT_TRUE(selection.get(3));
	ASSERT_FALSE(selection.get(4));
	ASSERT_TRUE(selection.get(5));

	selection.frob(3, BitOp::remove);
	ASSERT_EQ(selection.count_obj(), 2);
	ASSERT_TRUE(selection.get(2));
	ASSERT_FALSE(selection.get(3));
	ASSERT_TRUE(selection.get(5));

	selection.frob(3, BitOp::toggle);
	selection.frob(4, BitOp::toggle);
	selection.frob(5, BitOp::toggle);
	ASSERT_TRUE(selection.get(2));
	ASSERT_TRUE(selection.get(3));
	ASSERT_TRUE(selection.get(4));
	ASSERT_FALSE(selection.get(5));
}

TEST(MSelect, FrobRange)
{
	selection_c selection;

	selection.frob_range(1, 10, BitOp::add);
	ASSERT_FALSE(selection.get(0));
	ASSERT_TRUE(selection.get(1));
	ASSERT_TRUE(selection.get(2));
	ASSERT_TRUE(selection.get(3));
	ASSERT_TRUE(selection.get(4));
	ASSERT_TRUE(selection.get(5));
	ASSERT_TRUE(selection.get(6));
	ASSERT_TRUE(selection.get(7));
	ASSERT_TRUE(selection.get(8));
	ASSERT_TRUE(selection.get(9));
	ASSERT_TRUE(selection.get(10));

	selection.frob_range(3, 6, BitOp::remove);
	ASSERT_FALSE(selection.get(0));
	ASSERT_TRUE(selection.get(1));
	ASSERT_TRUE(selection.get(2));
	ASSERT_FALSE(selection.get(3));
	ASSERT_FALSE(selection.get(4));
	ASSERT_FALSE(selection.get(5));
	ASSERT_FALSE(selection.get(6));
	ASSERT_TRUE(selection.get(7));
	ASSERT_TRUE(selection.get(8));
	ASSERT_TRUE(selection.get(9));
	ASSERT_TRUE(selection.get(10));


	selection.frob_range(5, 9, BitOp::toggle);

	ASSERT_FALSE(selection.get(0));
	ASSERT_TRUE(selection.get(1));
	ASSERT_TRUE(selection.get(2));
	ASSERT_FALSE(selection.get(3));
	ASSERT_FALSE(selection.get(4));
	ASSERT_TRUE(selection.get(5));
	ASSERT_TRUE(selection.get(6));
	ASSERT_FALSE(selection.get(7));
	ASSERT_FALSE(selection.get(8));
	ASSERT_FALSE(selection.get(9));
	ASSERT_TRUE(selection.get(10));
}

TEST(MSelect, Merge)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);

	selection_c selection2;
	selection2.set(5);
	selection2.set(9);
	selection2.set(1);

	selection.merge(selection2);
	ASSERT_EQ(selection.count_obj(), 5);
	ASSERT_TRUE(selection.get(2));
	ASSERT_TRUE(selection.get(3));
	ASSERT_TRUE(selection.get(5));
	ASSERT_TRUE(selection.get(9));
	ASSERT_TRUE(selection.get(1));
}

TEST(MSelect, MergeAllowsDifferentTypes)
{
	selection_c selection(ObjType::things);
	selection.set(2);
	selection.set(3);
	selection.set(5);

	selection_c selection2(ObjType::vertices);
	selection2.set(5);
	selection2.set(9);
	selection2.set(1);

	selection.merge(selection2);
	ASSERT_EQ(selection.what_type(), ObjType::things);
	ASSERT_EQ(selection.count_obj(), 5);
	ASSERT_TRUE(selection.get(2));
	ASSERT_TRUE(selection.get(3));
	ASSERT_TRUE(selection.get(5));
	ASSERT_TRUE(selection.get(9));
	ASSERT_TRUE(selection.get(1));
}

TEST(MSelect, MergingExtendedSelections)
{
	selection_c selection(ObjType::things, true);
	selection.set_ext(2, 12);
	selection.set_ext(3, 23);
	selection.set_ext(5, 45);

	selection_c selection2(ObjType::things, true);
	selection2.set_ext(5, 67);
	selection2.set_ext(3, 89);
	selection2.set_ext(1, 90);

	selection.merge(selection2);
	ASSERT_EQ(selection.get_ext(2), 12);
	ASSERT_EQ(selection.get_ext(3), 95);	// OR between the values
	ASSERT_EQ(selection.get_ext(5), 111);	// OR between the values
	ASSERT_EQ(selection.get_ext(1), 90);
}

TEST(MSelect, Unmerge)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);

	selection_c selection2;
	selection2.set(5);
	selection2.set(3);
	selection2.set(1);

	selection.unmerge(selection2);
	ASSERT_EQ(selection.count_obj(), 1);
	ASSERT_TRUE(selection.get(2));

	// restore it
	selection.set(3);
	selection.set(5);

	selection2.unmerge(selection);
	ASSERT_EQ(selection2.count_obj(), 1);
	ASSERT_TRUE(selection2.get(1));
}

TEST(MSelect, Intersect)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);

	selection_c selection2;
	selection2.set(5);
	selection2.set(3);
	selection2.set(1);

	selection.intersect(selection2);

	ASSERT_EQ(selection.count_obj(), 2);
	ASSERT_TRUE(selection.get(3));
	ASSERT_TRUE(selection.get(5));
}

TEST(MSelect, DifferentTypesAreNotEqual)
{
	selection_c selection(ObjType::things);
	selection.set(2);
	selection.set(3);
	selection.set(5);

	selection_c selection2(ObjType::vertices);
	selection2.set(2);
	selection2.set(3);
	selection2.set(5);

	ASSERT_FALSE(selection.test_equal(selection2));
}

TEST(MSelect, Inequality)
{
	selection_c selection;
	selection.set(2);
	selection.set(5);

	selection_c selection2;
	selection2.set(2);
	selection2.set(3);
	selection2.set(5);

	ASSERT_FALSE(selection.test_equal(selection2));
}

TEST(MSelect, Equality)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);

	selection_c selection2;
	selection2.set(2);
	selection2.set(3);
	selection2.set(5);

	ASSERT_TRUE(selection.test_equal(selection2));
}

TEST(MSelect, FindFirstSecond)
{
	selection_c selection;
	ASSERT_EQ(selection.find_first(), -1);
	ASSERT_EQ(selection.find_second(), -1);

	selection.set(4);
	ASSERT_EQ(selection.find_first(), 4);
	ASSERT_EQ(selection.find_second(), -1);

	selection.set(2);
	ASSERT_EQ(selection.find_first(), 4);	// we must keep the first selected
	ASSERT_EQ(selection.find_second(), 2);

	selection.set(5);
	ASSERT_EQ(selection.find_first(), 4);
	ASSERT_EQ(selection.find_second(), 2);

	selection.clear(4);
	// Once clearing, all bets are off about order
	ASSERT_NE(selection.find_first(), -1);
	ASSERT_NE(selection.find_second(), -1);
	ASSERT_NE(selection.find_second(), selection.find_first());

	selection.clear(2);

	ASSERT_EQ(selection.find_first(), 5);
	ASSERT_EQ(selection.find_second(), -1);

}

TEST(MSelect, Iterator)
{
	selection_c selection;
	selection.set(2);
	selection.set(5);
	selection.set(9);
	selection.set(13);
	selection.set(7);
	selection.set(12);

	selection_c checkSelection;
	int runs = 0;
	for(sel_iter_c iter(selection); !iter.done(); iter.next())
	{
		++runs;
		checkSelection.set(*iter);
		ASSERT_EQ(checkSelection.count_obj(), runs);
		// Check that intersecting does nothing
		checkSelection.intersect(selection);
		ASSERT_EQ(checkSelection.count_obj(), runs);
	}

	// We ended up filling it
	ASSERT_TRUE(checkSelection.test_equal(selection));
}

TEST(MSelect, IteratorExtended)
{
	selection_c selection(ObjType::things, true);
	selection.set_ext(2, 12);
	selection.set_ext(5, 23);
	selection.set_ext(9, 34);
	selection.set_ext(13, 45);
	selection.set_ext(7, 56);
	selection.set_ext(12, 67);

	selection_c checkSelection(ObjType::things, true);
	int runs = 0;
	for(sel_iter_c iter(selection); !iter.done(); iter.next())
	{
		++runs;
		checkSelection.set_ext(*iter, selection.get_ext(*iter));
		ASSERT_EQ(checkSelection.count_obj(), runs);
		// Check that intersecting does nothing
		checkSelection.intersect(selection);
		ASSERT_EQ(checkSelection.count_obj(), runs);
	}

	// Now check we preserve the content
	ASSERT_EQ(checkSelection.get_ext(2), 12);
	ASSERT_EQ(checkSelection.get_ext(5), 23);
	ASSERT_EQ(checkSelection.get_ext(9), 34);
	ASSERT_EQ(checkSelection.get_ext(13), 45);
	ASSERT_EQ(checkSelection.get_ext(7), 56);
	ASSERT_EQ(checkSelection.get_ext(12), 67);
}

TEST(MSelect, InternalToBitvec)
{
	selection_c selection;
	for(int i = 0; i < MAX_STORE_SEL * 2; ++i)
		selection.set(i * i);

	// Check everything got in
	ASSERT_EQ(selection.count_obj(), MAX_STORE_SEL * 2);
	for(int i = 0; i < MAX_STORE_SEL * 2; ++i)
		ASSERT_TRUE(selection.get(i * i));
}

TEST(MSelect, BitvecRealloc)
{
	selection_c selection;
	for(int i = 0; i < 2048; ++i)
		selection.set(i);

	ASSERT_EQ(selection.count_obj(), 2048);
	for(int i = 0; i < 2048; ++i)
		ASSERT_TRUE(selection.get(i));
}

TEST(MSelect, ExtendedSizeCanGrow)
{
	selection_c selection(ObjType::things, true);
	// Must avoid setting ext to 0, so we still have 1024 selected
	for(int i = 0; i < 1024; ++i)
		selection.set_ext(i, static_cast<byte>((i * i + 1) % 256));

	ASSERT_EQ(selection.count_obj(), 1024);
	for(int i = 0; i < 1024; ++i)
		ASSERT_EQ(selection.get_ext(i), static_cast<byte>((i * i + 1) % 256));
}
