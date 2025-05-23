// Copyright (c) 2014-2022 Thomas Fussell
// Copyright (c) 2024-2025 xlnt-community
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE
//
// @license: http://www.opensource.org/licenses/mit-license.php
// @author: see AUTHORS file

#include <helpers/test_suite.hpp>
#include <xlnt/worksheet/worksheet.hpp>
#include <xlnt/workbook/workbook.hpp>
#include <xlnt/utils/path.hpp>

class drawing_test_suite : public test_suite
{
public:
    drawing_test_suite()
    {
        register_test(test_load_save);
    }

    void test_load_save()
    {
        xlnt::workbook wb1;
        wb1.load(path_helper::test_file("2_minimal.xlsx"));
        auto ws1 = wb1.active_sheet();
        xlnt_assert_equals(ws1.has_drawing(), false);

        xlnt::workbook wb2;
        wb2.load(path_helper::test_file("14_images.xlsx"));
        auto ws2 = wb2.active_sheet();
        xlnt_assert_equals(ws2.has_drawing(), true);
        wb2.save("temp_with_images.xlsx");

        xlnt::workbook wb3;
        wb3.load("temp_with_images.xlsx");
        auto ws3 = wb3.active_sheet();
        xlnt_assert_equals(ws3.has_drawing(), true);
    }
};
static drawing_test_suite x{};
