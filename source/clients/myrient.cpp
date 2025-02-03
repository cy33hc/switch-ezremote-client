#include <lexbor/html/parser.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/node.h>
#include <fstream>
#include <map>
#include "common.h"
#include "clients/remote_client.h"
#include "clients/myrient.h"
#include "lang.h"
#include "util.h"
#include "windows.h"


static std::map<std::string, int> month_map = {{"Jan", 1}, {"Feb", 2}, {"Mar", 3}, {"Apr", 4}, {"May", 5}, {"Jun", 6}, {"Jul", 7}, {"Aug", 8}, {"Sep", 9}, {"Oct", 10}, {"Nov", 11}, {"Dec", 12}};

std::vector<DirEntry> MyrientClient::ListDir(const std::string &path)
{
    CHTTPClient::HeadersMap headers;
    CHTTPClient::HttpResponse res;
    std::vector<DirEntry> out;
    DirEntry entry;
    Util::SetupPreviousFolder(path, &entry);
    out.push_back(entry);

    std::string encoded_path = this->host_url + CHTTPClient::EncodeUrl(GetFullPath(path)+"/");
    if (client->Get(encoded_path, headers, res))
    {
        lxb_status_t status;
        lxb_dom_attr_t *attr;
        lxb_dom_element_t *tr_element, *td_element;
        lxb_html_document_t *document;
        lxb_dom_collection_t *tr_collection;
        lxb_dom_collection_t *td_collection;
        std::string tmp_string;
        const lxb_char_t *value;
        size_t value_len;
        std::string lower_filter = Util::ToLower(remote_filter);

        // Extract first 100 tr in the <table id="list"> element
        std::string res_body = std::string(res.strBody.data());
        res.strBody.clear();
        size_t start_parse_pos = res_body.find("<table id=\"list\">");
        size_t table_list_end_pos = res_body.find("</table>", start_parse_pos);

        int rows = Util::CountOccurrence(res_body, "<tr>", start_parse_pos, table_list_end_pos);
        if (apply_native_filter_state == 2 && rows > 500)
        {
            selected_action = ACTION_APPLY_REMOTE_NATIVE_FILTER;
            return out;
        }

        start_parse_pos = Util::NthOccurrence(res_body, "<tr>", 3, start_parse_pos, table_list_end_pos);
        size_t tr_nth_start_pos = Util::NthOccurrence(res_body, "<tr>", 100, start_parse_pos, table_list_end_pos);
        size_t tr_nth_end_pos = res_body.find("</tr>", tr_nth_start_pos) + 5;;

        while (tr_nth_start_pos != std::string::npos)
        {
            std::string temp_str = "<html><body><table><tbody>" + res_body.substr(start_parse_pos, tr_nth_end_pos-start_parse_pos) + "</tbody></table></body></html>";

            document = lxb_html_document_create();
            status = lxb_html_document_parse(document, (lxb_char_t *)temp_str.c_str(), temp_str.length());
            if (status != LXB_STATUS_OK)
            {
                lxb_html_document_destroy(document);
                goto finish;
            }

            tr_collection = lxb_dom_collection_make(&document->dom_document, 128);
            if (tr_collection == NULL)
            {
                lxb_html_document_destroy(document);
                goto finish;
            }

            status = lxb_dom_elements_by_tag_name(lxb_dom_interface_element(document->body),
                                                tr_collection, (const lxb_char_t *)"tr", 2);
            if (status != LXB_STATUS_OK && lxb_dom_collection_length(tr_collection) < 2)
            {
                lxb_dom_collection_destroy(tr_collection, true);
                lxb_html_document_destroy(document);
                goto finish;
            }

            // skip row 0 , since it has the previous folder header
            for (size_t i = 0; i < lxb_dom_collection_length(tr_collection); i++)
            {
                DirEntry entry;
                std::string title, aclass;
                memset(&entry.modified, 0, sizeof(DateTime));

                tr_element = lxb_dom_collection_element(tr_collection, i);

                td_collection = lxb_dom_collection_make(&document->dom_document, 5);
                status = lxb_dom_elements_by_tag_name(tr_element,
                                                    td_collection, (const lxb_char_t *)"td", 2);
                if (status != LXB_STATUS_OK || lxb_dom_collection_length(td_collection) < 3)
                {
                    lxb_dom_collection_destroy(td_collection, true);
                    lxb_dom_collection_destroy(tr_collection, true);
                    lxb_html_document_destroy(document);
                    goto finish;
                }

                // td0 contains the <a> tag
                td_element = lxb_dom_collection_element(td_collection, 0);
                lxb_dom_node_t *a_node = Util::NextChildElement(td_element);
                value = lxb_dom_element_local_name(lxb_dom_interface_element(a_node), &value_len);
                tmp_string = std::string((const char *)value, value_len);
                if (tmp_string.compare("a") != 0)
                {
                    lxb_dom_collection_destroy(td_collection, true);
                    lxb_dom_collection_destroy(tr_collection, true);
                    lxb_html_document_destroy(document);
                    goto finish;
                }
                value = lxb_dom_element_get_attribute(lxb_dom_interface_element(a_node), (const lxb_char_t *)"href", 4, &value_len);
                tmp_string = std::string((const char *)value, value_len);
                if (tmp_string[tmp_string.length()-1] == '/')
                    tmp_string = tmp_string.substr(0, tmp_string.length()-1);
                tmp_string = BaseClient::UnEscape(tmp_string);
                sprintf(entry.name, "%s", tmp_string.c_str());
                sprintf(entry.directory, "%s", path.c_str());
                if (path.length() > 0 && path[path.length() - 1] == '/')
                {
                    sprintf(entry.path, "%s%s", path.c_str(), entry.name);
                }
                else
                {
                    sprintf(entry.path, "%s/%s", path.c_str(), entry.name);
                }

                if (apply_native_filter_state == 1)
                {
                    std::string temp_name = Util::ToLower(entry.name);
                    if (lower_filter.length() > 0 && temp_name.find(lower_filter) == std::string::npos)
                    {
                        lxb_dom_collection_destroy(td_collection, true);
                        continue;
                    }
                }

                // next td contains file size, if fize size is "-", then it's a directory
                td_element = lxb_dom_collection_element(td_collection, 1);
                value = lxb_dom_node_text_content(Util::NextChildTextNode(td_element), &value_len);
                tmp_string = std::string((const char *)value, value_len);

                if (tmp_string.compare("-") == 0)
                {
                    entry.isDir = true;
                    entry.selectable = true;
                    entry.file_size = 0;
                    sprintf(entry.display_size, "%s", lang_strings[STR_FOLDER]);
                }
                else
                {
                    entry.isDir = false;
                    entry.selectable = true;
                    uint64_t multiplier = 1;
                    std::vector<std::string> fsize_parts = Util::Split(tmp_string, " ");

                    float fsize = atof(fsize_parts[0].c_str());

                    if (fsize_parts.size() > 1)
                    {
                        switch (fsize_parts[1][0])
                        {
                            case 'K':
                                multiplier = 1024;
                                break;
                            case 'M':
                                multiplier = 1048576;
                                break;
                            case 'G':
                                multiplier = 1073741824;
                                break;
                            default:
                                multiplier = 1;
                        }
                    }
                    entry.file_size = fsize * multiplier;
                    DirEntry::SetDisplaySize(&entry);
                }

                // next td contains the date
                td_element = lxb_dom_collection_element(td_collection, 2);
                value = lxb_dom_node_text_content(Util::NextChildTextNode(td_element), &value_len);
                tmp_string = std::string((const char *)value, value_len);
                std::vector<std::string> date_time = Util::Split(tmp_string, " ");

                if (date_time.size() > 1)
                {
                    std::vector<std::string> adate = Util::Split(date_time[0], "-");
                    if (adate.size() == 3)
                    {
                        entry.modified.day = atoi(adate[0].c_str());
                        entry.modified.month = month_map[adate[1]];
                        entry.modified.year = atoi(adate[2].c_str());
                    }

                    std::vector<std::string> atime = Util::Split(date_time[1], ":");
                    if (atime.size() == 2)
                    {
                        entry.modified.hours = atoi(atime[0].c_str());
                        entry.modified.minutes = atoi(atime[1].c_str());
                    }
                }

                lxb_dom_collection_destroy(td_collection, true);
                out.push_back(entry);
            }

            lxb_dom_collection_destroy(tr_collection, true);
            lxb_html_document_destroy(document);
            temp_str.clear();

            start_parse_pos =tr_nth_end_pos+1;
            tr_nth_start_pos = Util::NthOccurrence(res_body, "<tr>", 100, start_parse_pos, table_list_end_pos);
            tr_nth_end_pos = res_body.find("</tr>", tr_nth_start_pos)+5;
        }
        res_body.clear();
    }
    else
    {
        sprintf(this->response, "%s", res.errMessage.c_str());
        return out;
    }

finish:
    apply_native_filter_state = 2;
    return out;
}