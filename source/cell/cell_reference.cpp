#include <locale>

#include <xlnt/cell/cell_reference.hpp>
#include <xlnt/utils/exceptions.hpp>
#include <xlnt/worksheet/range_reference.hpp>

#include <detail/constants.hpp>

namespace xlnt {

std::size_t cell_reference_hash::operator()(const cell_reference &k) const
{
    return k.get_row() * constants::MaxColumn().index + k.get_column_index().index;
}

cell_reference &cell_reference::make_absolute(bool absolute_column, bool absolute_row)
{
    column_absolute(absolute_column);
    row_absolute(absolute_row);
    
    return *this;
}

cell_reference::cell_reference() : cell_reference(1, 1)
{
}

cell_reference::cell_reference(const string &string)
{
    auto split = split_reference(string, absolute_column_, absolute_row_);
    
    set_column(split.first);
    set_row(split.second);
}

cell_reference::cell_reference(const char *reference_string)
    : cell_reference(string(reference_string))
{
}

cell_reference::cell_reference(const string &column, row_t row)
    : cell_reference(column_t(column), row)
{
}

cell_reference::cell_reference(column_t column_index, row_t row)
    : column_(column_index), row_(row), absolute_row_(false), absolute_column_(false)
{
    if (row_ == 0 || !(row_ <= constants::MaxRow()) || column_ == 0 || !(column_ <= constants::MaxColumn()))
    {
        throw cell_coordinates_exception(column_, row_);
    }
}

range_reference cell_reference::operator, (const xlnt::cell_reference &other) const
{
    return range_reference(*this, other);
}

string cell_reference::to_string() const
{
    string string_representation;
    
    if (absolute_column_)
    {
        string_representation.append("$");
    }
    
    string_representation.append(column_.column_string());
    
    if (absolute_row_)
    {
        string_representation.append("$");
    }
    
    string_representation.append(string::from(row_));

    return string_representation;
}

range_reference cell_reference::to_range() const
{
    return range_reference(column_, row_, column_, row_);
}

std::pair<string, row_t> cell_reference::split_reference(const string &reference_string,
                                                              bool &absolute_column, bool &absolute_row)
{
    absolute_column = false;
    absolute_row = false;

    // Convert a coordinate string like 'B12' to a tuple ('B', 12)
    bool column_part = true;

    string column_string;

    for (auto character : reference_string)
    {
        auto upper = std::toupper(character.get(), std::locale::classic());

        if (std::isalpha(character.get(), std::locale::classic()))
        {
            if (column_part)
            {
                column_string.append(upper);
            }
            else
            {
                throw cell_coordinates_exception(reference_string);
            }
        }
        else if (character == '$')
        {
            if (column_part)
            {
                if (column_string.empty())
                {
                    column_string.append(upper);
                }
                else
                {
                    column_part = false;
                }
            }
        }
        else
        {
            if (column_part)
            {
                column_part = false;
            }
            else if (!std::isdigit(character.get(), std::locale::classic()))
            {
                throw cell_coordinates_exception(reference_string);
            }
        }
    }

    string row_string = reference_string.substr(column_string.length());

    if (row_string.length() == 0)
    {
        throw cell_coordinates_exception(reference_string);
    }

    if (column_string[0] == '$')
    {
        absolute_row = true;
        column_string = column_string.substr(1);
    }

    if (row_string[0] == '$')
    {
        absolute_column = true;
        row_string = row_string.substr(1);
    }

    return { column_string, row_string.to<row_t>() };
}

cell_reference cell_reference::make_offset(int column_offset, int row_offset) const
{
    //TODO: check for overflow/underflow
    return cell_reference(static_cast<column_t>(static_cast<int>(column_.index) + column_offset),
                          static_cast<row_t>(static_cast<int>(row_) + row_offset));
}

bool cell_reference::operator==(const cell_reference &comparand) const
{
    return comparand.column_ == column_ &&
        comparand.row_ == row_ &&
        absolute_column_ == comparand.absolute_column_ &&
        absolute_row_ == comparand.absolute_row_;
}

bool cell_reference::operator<(const cell_reference &other)
{
    if (row_ != other.row_)
    {
        return row_ < other.row_;
    }

    return column_ < other.column_;
}

bool cell_reference::operator>(const cell_reference &other)
{
    if (row_ != other.row_)
    {
        return row_ > other.row_;
    }

    return column_ > other.column_;
}

bool cell_reference::operator<=(const cell_reference &other)
{
    if (row_ != other.row_)
    {
        return row_ < other.row_;
    }

    return column_ <= other.column_;
}

bool cell_reference::operator>=(const cell_reference &other)
{
    if (row_ != other.row_)
    {
        return row_ > other.row_;
    }

    return column_ >= other.column_;
}

} // namespace xlnt
