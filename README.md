# Atlassian Field Data Scraper

This repository contains tools to scrape and organize field data from Atlassian wiki pages into a structured format.

## Problem
You need to extract field information from three sections of an Atlassian wiki page:
- Initial Quote Fields
- Finalize Quote Fields  
- Bind Quote Fields

## Desired Output Format
The data should be organized into columns:
1. **Field Name**: Hierarchical field name (e.g., "Named Insureds - Locations - Address - Zip Code")
2. **JSON Path**: Location within JSON structure (e.g., "namedInsureds[*].locations[*].address.zipCode")
3. **Required for Quote**: Boolean indicating if field is required for quotes
4. **Required for Bind**: Boolean indicating if field is required for binding
5. **Section**: Which section the field comes from (Initial, Finalize, or Bind)
6. **Description**: Plain English description of what the field represents

## Available Tools

### 1. Automated Scraper (`scrape_atlassian_fields.py`)
Attempts to automatically scrape the Atlassian wiki page. Requires:
- Network access to the Atlassian instance
- Authentication credentials (if the page is private)

### 2. Manual Data Processor (`field_data_processor.py`)
Helps structure manually copied data into the desired format.

## Usage Options

### Option 1: Manual Data Entry
If you can copy the content from the wiki pages or PDF:

1. Run the processor:
```bash
python3 field_data_processor.py
```

2. Modify the script to add your actual field data in the `example_usage()` function.

### Option 2: Provide Authentication
If you have Atlassian credentials:

1. Edit `scrape_atlassian_fields.py`
2. Add your username and password/token:
```python
scraper = AtlassianFieldScraper(base_url, username="your_username", password="your_token")
```

### Option 3: Copy Page Content
If you can copy the HTML content from the browser:

1. Save the page HTML to a file
2. Modify the scraper to read from the local file instead of making HTTP requests

## Output Files
The tools will generate:
- `master_field_list.csv` - Structured data in CSV format
- `master_field_list.json` - Structured data in JSON format

## Example Output
```csv
field_name,json_path,required_quote,required_bind,section,description
"Named Insureds - Locations - Address - Zip Code","namedInsureds[*].locations[*].address.zipCode",true,true,"Initial","The postal code for the insured location"
"Policy - Effective Date","policy.effectiveDate",true,true,"Finalize","The date when the policy becomes effective"
```

## Next Steps
Since the automated scraping requires authentication, you can:

1. **Provide the PDF**: If you can share the PDF content, I can help extract the data
2. **Copy the HTML**: Copy the page source from your browser and save it as an HTML file
3. **Manual entry**: Use the field processor to manually enter the field data
4. **Provide credentials**: If you have Atlassian access, provide credentials for automated scraping

Let me know which approach you'd prefer!