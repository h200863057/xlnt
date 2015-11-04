#include <algorithm>

#include <xlnt/packaging/document_properties.hpp>
#include <xlnt/packaging/manifest.hpp>
#include <xlnt/packaging/relationship.hpp>
#include <xlnt/serialization/workbook_serializer.hpp>
#include <xlnt/serialization/xml_document.hpp>
#include <xlnt/serialization/xml_node.hpp>
#include <xlnt/utils/datetime.hpp>
#include <xlnt/utils/exceptions.hpp>
#include <xlnt/workbook/named_range.hpp>
#include <xlnt/workbook/workbook.hpp>
#include <xlnt/worksheet/range_reference.hpp>
#include <xlnt/worksheet/worksheet.hpp>

#include <detail/constants.hpp>

namespace {

xlnt::datetime w3cdtf_to_datetime(const xlnt::string &string)
{
    xlnt::datetime result(1900, 1, 1);

    auto separator_index = string.find('-');
    result.year = string.substr(0, separator_index).to<int>();
    result.month = string.substr(separator_index + 1, string.find('-', separator_index + 1)).to<int>();
    separator_index = string.find('-', separator_index + 1);
    result.day = string.substr(separator_index + 1, string.find('T', separator_index + 1)).to<int>();
    separator_index = string.find('T', separator_index + 1);
    result.hour = string.substr(separator_index + 1, string.find(':', separator_index + 1)).to<int>();
    separator_index = string.find(':', separator_index + 1);
    result.minute = string.substr(separator_index + 1, string.find(':', separator_index + 1)).to<int>();
    separator_index = string.find(':', separator_index + 1);
    result.second = string.substr(separator_index + 1, string.find('Z', separator_index + 1)).to<int>();

    return result;
}

xlnt::string fill(const xlnt::string &str, std::size_t length = 2)
{
    if (str.length() >= length)
    {
        return str;
    }

	xlnt::string result;

	for (std::size_t i = 0; i < length - str.length(); i++)
	{
		result.append('0');
	}

    return result + str;
}

xlnt::string datetime_to_w3cdtf(const xlnt::datetime &dt)
{
    return xlnt::string::from(dt.year) + "-" + fill(xlnt::string::from(dt.month)) + "-" + fill(xlnt::string::from(dt.day)) + "T" +
           fill(xlnt::string::from(dt.hour)) + ":" + fill(xlnt::string::from(dt.minute)) + ":" +
           fill(xlnt::string::from(dt.second)) + "Z";
}

} // namespace

namespace xlnt {

workbook_serializer::workbook_serializer(workbook &wb) : workbook_(wb)
{
}

void workbook_serializer::read_properties_core(const xml_document &xml)
{
    auto &props = workbook_.get_properties();
    auto root_node = xml.get_child("cp:coreProperties");

    props.excel_base_date = calendar::windows_1900;

    if (root_node.has_child("dc:creator"))
    {
        props.creator = root_node.get_child("dc:creator").get_text();
    }
    if (root_node.has_child("cp:lastModifiedBy"))
    {
        props.last_modified_by = root_node.get_child("cp:lastModifiedBy").get_text();
    }
    if (root_node.has_child("dcterms:created"))
    {
        string created_string = root_node.get_child("dcterms:created").get_text();
        props.created = w3cdtf_to_datetime(created_string);
    }
    if (root_node.has_child("dcterms:modified"))
    {
        string modified_string = root_node.get_child("dcterms:modified").get_text();
        props.modified = w3cdtf_to_datetime(modified_string);
    }
}

xml_document workbook_serializer::write_properties_core() const
{
    auto &props = workbook_.get_properties();

    xml_document xml;

    auto root_node = xml.add_child("cp:coreProperties");

    xml.add_namespace("cp", "http://schemas.openxmlformats.org/package/2006/metadata/core-properties");
    xml.add_namespace("dc", "http://purl.org/dc/elements/1.1/");
    xml.add_namespace("dcmitype", "http://purl.org/dc/dcmitype/");
    xml.add_namespace("dcterms", "http://purl.org/dc/terms/");
    xml.add_namespace("xsi", "http://www.w3.org/2001/XMLSchema-instance");

    root_node.add_child("dc:creator").set_text(props.creator);
    root_node.add_child("cp:lastModifiedBy").set_text(props.last_modified_by);
    root_node.add_child("dcterms:created").set_text(datetime_to_w3cdtf(props.created));
    root_node.get_child("dcterms:created").add_attribute("xsi:type", "dcterms:W3CDTF");
    root_node.add_child("dcterms:modified").set_text(datetime_to_w3cdtf(props.modified));
    root_node.get_child("dcterms:modified").add_attribute("xsi:type", "dcterms:W3CDTF");
    root_node.add_child("dc:title").set_text(props.title);
    root_node.add_child("dc:description");
    root_node.add_child("dc:subject");
    root_node.add_child("cp:keywords");
    root_node.add_child("cp:category");

    return xml;
}

xml_document workbook_serializer::write_properties_app() const
{
    xml_document xml;

    auto root_node = xml.add_child("Properties");

    xml.add_namespace("", "http://schemas.openxmlformats.org/officeDocument/2006/extended-properties");
    xml.add_namespace("vt", "http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes");

    root_node.add_child("Application").set_text("Microsoft Excel");
    root_node.add_child("DocSecurity").set_text("0");
    root_node.add_child("ScaleCrop").set_text("false");
    root_node.add_child("Company");
    root_node.add_child("LinksUpToDate").set_text("false");
    root_node.add_child("SharedDoc").set_text("false");
    root_node.add_child("HyperlinksChanged").set_text("false");
    root_node.add_child("AppVersion").set_text("12.0000");

    auto heading_pairs_node = root_node.add_child("HeadingPairs");
    auto heading_pairs_vector_node = heading_pairs_node.add_child("vt:vector");
    heading_pairs_vector_node.add_attribute("baseType", "variant");
    heading_pairs_vector_node.add_attribute("size", "2");
    heading_pairs_vector_node.add_child("vt:variant").add_child("vt:lpstr").set_text("Worksheets");
    heading_pairs_vector_node.add_child("vt:variant")
        .add_child("vt:i4")
        .set_text(string::from(workbook_.get_sheet_names().size()));

    auto titles_of_parts_node = root_node.add_child("TitlesOfParts");
    auto titles_of_parts_vector_node = titles_of_parts_node.add_child("vt:vector");
    titles_of_parts_vector_node.add_attribute("baseType", "lpstr");
    titles_of_parts_vector_node.add_attribute("size", string::from(workbook_.get_sheet_names().size()));

    for (auto ws : workbook_)
    {
        titles_of_parts_vector_node.add_child("vt:lpstr").set_text(ws.get_title());
    }

    return xml;
}

xml_document workbook_serializer::write_workbook() const
{
    std::size_t num_visible = 0;

    for (auto ws : workbook_)
    {
        if (ws.get_page_setup().get_sheet_state() == xlnt::page_setup::sheet_state::visible)
        {
            num_visible++;
        }
    }

    if (num_visible == 0)
    {
        throw xlnt::value_error();
    }

    xml_document xml;

    auto root_node = xml.add_child("workbook");

    xml.add_namespace("", "http://schemas.openxmlformats.org/spreadsheetml/2006/main");
    xml.add_namespace("r", "http://schemas.openxmlformats.org/officeDocument/2006/relationships");

    auto file_version_node = root_node.add_child("fileVersion");
    file_version_node.add_attribute("appName", "xl");
    file_version_node.add_attribute("lastEdited", "4");
    file_version_node.add_attribute("lowestEdited", "4");
    file_version_node.add_attribute("rupBuild", "4505");

    auto workbook_pr_node = root_node.add_child("workbookPr");
    workbook_pr_node.add_attribute("codeName", "ThisWorkbook");
    workbook_pr_node.add_attribute("defaultThemeVersion", "124226");
    workbook_pr_node.add_attribute("date1904",
                                   workbook_.get_properties().excel_base_date == calendar::mac_1904 ? "1" : "0");

    auto book_views_node = root_node.add_child("bookViews");
    auto workbook_view_node = book_views_node.add_child("workbookView");
    workbook_view_node.add_attribute("activeTab", "0");
    workbook_view_node.add_attribute("autoFilterDateGrouping", "1");
    workbook_view_node.add_attribute("firstSheet", "0");
    workbook_view_node.add_attribute("minimized", "0");
    workbook_view_node.add_attribute("showHorizontalScroll", "1");
    workbook_view_node.add_attribute("showSheetTabs", "1");
    workbook_view_node.add_attribute("showVerticalScroll", "1");
    workbook_view_node.add_attribute("tabRatio", "600");
    workbook_view_node.add_attribute("visibility", "visible");

    auto sheets_node = root_node.add_child("sheets");
    auto defined_names_node = root_node.add_child("definedNames");

    for (const auto &relationship : workbook_.get_relationships())
    {
        if (relationship.get_type() == relationship::type::worksheet)
        {
            // TODO: this is ugly
            string sheet_index_string = relationship.get_target_uri();
            sheet_index_string = sheet_index_string.substr(0, sheet_index_string.find('.'));
            sheet_index_string = sheet_index_string.substr(sheet_index_string.find_last_of('/'));
            auto iter = sheet_index_string.end();
            --iter;
            while (isdigit((*iter).get()))
                --iter;
            auto first_digit = iter - sheet_index_string.begin();
            sheet_index_string = sheet_index_string.substr(static_cast<string::size_type>(first_digit + 1));
            std::size_t sheet_index = sheet_index_string.to<std::size_t>() - 1;

            auto ws = workbook_.get_sheet_by_index(sheet_index);

            auto sheet_node = sheets_node.add_child("sheet");
            sheet_node.add_attribute("name", ws.get_title());
            sheet_node.add_attribute("sheetId", string::from(sheet_index + 1));
            sheet_node.add_attribute("r:id", relationship.get_id());

            if (ws.has_auto_filter())
            {
                auto defined_name_node = defined_names_node.add_child("definedName");
                defined_name_node.add_attribute("name", "_xlnm._FilterDatabase");
                defined_name_node.add_attribute("hidden", "1");
                defined_name_node.add_attribute("localSheetId", "0");
                string name =
                    "'" + ws.get_title() + "'!" + range_reference::make_absolute(ws.get_auto_filter()).to_string();
                defined_name_node.set_text(name);
            }
        }
    }

    auto calc_pr_node = root_node.add_child("calcPr");
    calc_pr_node.add_attribute("calcId", "124519");
    calc_pr_node.add_attribute("calcMode", "auto");
    calc_pr_node.add_attribute("fullCalcOnLoad", "1");

    return xml;
}

xml_node workbook_serializer::write_named_ranges() const
{
    xlnt::xml_node named_ranges_node;

    for (auto &named_range : workbook_.get_named_ranges())
    {
        named_ranges_node.add_child(named_range.get_name());
    }

    return named_ranges_node;
}

} // namespace xlnt
