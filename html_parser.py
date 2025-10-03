#!/usr/bin/env python3
"""
HTML parser to extract field data from copied Atlassian wiki content.
This can help if you copy the page HTML and save it to a file.
"""

from bs4 import BeautifulSoup
import re
from field_data_processor import FieldDataProcessor

class AtlassianHTMLParser:
    def __init__(self):
        self.processor = FieldDataProcessor()
    
    def parse_html_file(self, html_file_path: str):
        """Parse HTML file containing Atlassian wiki content."""
        try:
            with open(html_file_path, 'r', encoding='utf-8') as file:
                html_content = file.read()
            
            soup = BeautifulSoup(html_content, 'html.parser')
            return self.extract_fields_from_soup(soup)
        except FileNotFoundError:
            print(f"File {html_file_path} not found.")
            return []
        except Exception as e:
            print(f"Error parsing HTML file: {e}")
            return []
    
    def parse_html_content(self, html_content: str):
        """Parse HTML content string."""
        soup = BeautifulSoup(html_content, 'html.parser')
        return self.extract_fields_from_soup(soup)
    
    def extract_fields_from_soup(self, soup: BeautifulSoup):
        """Extract fields from BeautifulSoup object."""
        sections = ['Initial', 'Finalize', 'Bind']
        all_fields = []
        
        for section in sections:
            print(f"Processing {section} section...")
            fields = self.extract_section_fields(soup, section)
            all_fields.extend(fields)
            print(f"Found {len(fields)} fields in {section} section")
        
        return all_fields
    
    def extract_section_fields(self, soup: BeautifulSoup, section_name: str):
        """Extract fields from a specific section."""
        fields = []
        
        # Look for section headers (various possible formats)
        section_patterns = [
            f"{section_name}.*[Qq]uote.*[Ff]ields",
            f"{section_name}.*[Ff]ields",
            f"{section_name}.*[Qq]uote",
            section_name
        ]
        
        section_header = None
        for pattern in section_patterns:
            section_header = soup.find(['h1', 'h2', 'h3', 'h4', 'h5', 'h6'], 
                                     string=re.compile(pattern, re.IGNORECASE))
            if section_header:
                break
        
        if not section_header:
            print(f"Section '{section_name}' not found")
            return fields
        
        # Find content after the header
        content = section_header.find_next(['table', 'div', 'ul', 'ol'])
        
        if content:
            if content.name == 'table':
                fields = self.extract_from_table(content, section_name)
            elif content.name in ['ul', 'ol']:
                fields = self.extract_from_list(content, section_name)
            elif content.name == 'div':
                # Look for nested tables or lists
                table = content.find('table')
                if table:
                    fields = self.extract_from_table(table, section_name)
                else:
                    ul = content.find(['ul', 'ol'])
                    if ul:
                        fields = self.extract_from_list(ul, section_name)
        
        return fields
    
    def extract_from_table(self, table, section_name: str):
        """Extract fields from a table."""
        fields = []
        rows = table.find_all('tr')
        
        if not rows:
            return fields
        
        # Get headers
        headers = []
        header_row = rows[0]
        for cell in header_row.find_all(['th', 'td']):
            headers.append(cell.get_text(strip=True).lower())
        
        # Process data rows
        for row in rows[1:]:
            cells = row.find_all(['td', 'th'])
            if len(cells) >= 2:
                field_name = cells[0].get_text(strip=True)
                description = cells[1].get_text(strip=True) if len(cells) > 1 else ""
                
                # Determine requirements based on headers or content
                required_quote = self.determine_requirement(cells, headers, 'quote')
                required_bind = self.determine_requirement(cells, headers, 'bind')
                
                if field_name:
                    field_data = self.processor.add_field(
                        field_name=field_name,
                        description=description,
                        section=section_name,
                        required_quote=required_quote,
                        required_bind=required_bind
                    )
                    fields.append(field_data)
        
        return fields
    
    def extract_from_list(self, list_element, section_name: str):
        """Extract fields from a list."""
        fields = []
        items = list_element.find_all('li')
        
        for item in items:
            text = item.get_text(strip=True)
            
            # Try to parse field name and description
            # Common patterns: "Field Name: Description" or "Field Name - Description"
            if ':' in text:
                parts = text.split(':', 1)
                field_name = parts[0].strip()
                description = parts[1].strip()
            elif ' - ' in text:
                parts = text.split(' - ', 1)
                field_name = parts[0].strip()
                description = parts[1].strip()
            else:
                field_name = text
                description = ""
            
            # Determine requirements (look for keywords in text)
            required_quote = 'required' in text.lower() and 'quote' in text.lower()
            required_bind = 'required' in text.lower() and 'bind' in text.lower()
            
            if field_name:
                field_data = self.processor.add_field(
                    field_name=field_name,
                    description=description,
                    section=section_name,
                    required_quote=required_quote,
                    required_bind=required_bind
                )
                fields.append(field_data)
        
        return fields
    
    def determine_requirement(self, cells, headers, requirement_type):
        """Determine if a field is required based on table structure."""
        # Look for requirement columns
        req_columns = []
        for i, header in enumerate(headers):
            if requirement_type in header.lower():
                req_columns.append(i)
        
        # Check the requirement columns
        for col_idx in req_columns:
            if col_idx < len(cells):
                cell_text = cells[col_idx].get_text(strip=True).lower()
                if any(word in cell_text for word in ['yes', 'true', 'required', 'mandatory', 'x', '✓']):
                    return True
        
        return False
    
    def save_results(self, filename_prefix='parsed_fields'):
        """Save the parsed results."""
        self.processor.save_to_csv(f'{filename_prefix}.csv')
        self.processor.save_to_json(f'{filename_prefix}.json')
        self.processor.print_sample()

def main():
    """Main function for HTML parsing."""
    parser = AtlassianHTMLParser()
    
    print("=== Atlassian HTML Parser ===")
    print("This tool can parse HTML content from Atlassian wiki pages.")
    print("\nOptions:")
    print("1. Parse HTML file")
    print("2. Parse HTML content (paste content)")
    print("3. Exit")
    
    choice = input("\nEnter your choice (1-3): ").strip()
    
    if choice == '1':
        file_path = input("Enter path to HTML file: ").strip()
        fields = parser.parse_html_file(file_path)
        if fields:
            parser.save_results()
            print(f"\nParsed {len(fields)} fields successfully!")
    
    elif choice == '2':
        print("Paste the HTML content below (press Ctrl+D when done):")
        html_content = ""
        try:
            while True:
                line = input()
                html_content += line + "\n"
        except EOFError:
            pass
        
        fields = parser.parse_html_content(html_content)
        if fields:
            parser.save_results()
            print(f"\nParsed {len(fields)} fields successfully!")
    
    elif choice == '3':
        print("Goodbye!")
    
    else:
        print("Invalid choice.")

if __name__ == "__main__":
    main()