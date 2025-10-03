#!/usr/bin/env python3
"""
Script to scrape Atlassian wiki pages for Master Field List data
and organize it into the requested format.
"""

import requests
from bs4 import BeautifulSoup
import json
import csv
import re
from typing import List, Dict, Any

class AtlassianFieldScraper:
    def __init__(self, base_url: str, username: str = None, password: str = None):
        """
        Initialize the scraper with Atlassian credentials if needed.
        
        Args:
            base_url: Base URL of the Atlassian instance
            username: Username for authentication (if required)
            password: Password or API token for authentication (if required)
        """
        self.base_url = base_url
        self.session = requests.Session()
        
        if username and password:
            self.session.auth = (username, password)
    
    def scrape_page(self, page_url: str) -> BeautifulSoup:
        """Scrape a single Atlassian page and return parsed HTML."""
        try:
            response = self.session.get(page_url)
            response.raise_for_status()
            return BeautifulSoup(response.content, 'html.parser')
        except requests.RequestException as e:
            print(f"Error scraping {page_url}: {e}")
            return None
    
    def extract_field_data(self, soup: BeautifulSoup, section_name: str) -> List[Dict[str, Any]]:
        """
        Extract field data from a specific section of the page.
        
        Args:
            soup: BeautifulSoup object of the page
            section_name: Name of the section (Initial, Finalize, Bind)
        
        Returns:
            List of dictionaries containing field information
        """
        fields = []
        
        # Look for the section header
        section_header = soup.find(['h1', 'h2', 'h3', 'h4', 'h5', 'h6'], 
                                 string=re.compile(section_name, re.IGNORECASE))
        
        if not section_header:
            print(f"Section '{section_name}' not found")
            return fields
        
        # Find the table or content after the header
        content = section_header.find_next(['table', 'div'])
        
        if content and content.name == 'table':
            # Extract data from table
            rows = content.find_all('tr')
            headers = []
            
            # Get headers from first row
            if rows:
                header_row = rows[0]
                headers = [th.get_text(strip=True) for th in header_row.find_all(['th', 'td'])]
            
            # Process data rows
            for row in rows[1:]:
                cells = row.find_all(['td', 'th'])
                if len(cells) >= 2:  # At least field name and description
                    field_data = {
                        'field_name': cells[0].get_text(strip=True),
                        'description': cells[1].get_text(strip=True) if len(cells) > 1 else '',
                        'section': section_name,
                        'required_quote': False,
                        'required_bind': False,
                        'json_path': ''
                    }
                    
                    # Try to extract additional information from other columns
                    if len(cells) > 2:
                        # Check for required indicators
                        for i, cell in enumerate(cells[2:], 2):
                            cell_text = cell.get_text(strip=True).lower()
                            if 'required' in cell_text or 'mandatory' in cell_text:
                                if 'quote' in cell_text:
                                    field_data['required_quote'] = True
                                if 'bind' in cell_text:
                                    field_data['required_bind'] = True
                    
                    fields.append(field_data)
        
        return fields
    
    def generate_json_path(self, field_name: str) -> str:
        """
        Generate JSON path based on field name hierarchy.
        
        Args:
            field_name: Field name in format "Parent - Child - Grandchild"
        
        Returns:
            JSON path string
        """
        # Split by common separators
        parts = re.split(r'\s*-\s*|\s*>\s*|\s*\.\s*', field_name)
        
        # Convert to camelCase and handle special cases
        json_parts = []
        for part in parts:
            # Clean up the part
            part = part.strip()
            if not part:
                continue
                
            # Convert to camelCase
            words = part.split()
            if len(words) == 1:
                json_parts.append(words[0].lower())
            else:
                camel_case = words[0].lower() + ''.join(word.capitalize() for word in words[1:])
                json_parts.append(camel_case)
        
        # Handle special cases and arrays
        json_path = '.'.join(json_parts)
        
        # Add array notation for common patterns
        if 'locations' in json_path.lower():
            json_path = json_path.replace('locations', 'locations[*]')
        if 'namedinsureds' in json_path.lower():
            json_path = json_path.replace('namedinsureds', 'namedInsureds[*]')
        if 'address' in json_path.lower():
            json_path = json_path.replace('address', 'address')
        
        return json_path
    
    def scrape_all_sections(self, page_url: str) -> List[Dict[str, Any]]:
        """Scrape all three sections from the Master Field List page."""
        soup = self.scrape_page(page_url)
        if not soup:
            return []
        
        all_fields = []
        sections = ['Initial', 'Finalize', 'Bind']
        
        for section in sections:
            fields = self.extract_field_data(soup, section)
            all_fields.extend(fields)
        
        # Generate JSON paths for all fields
        for field in all_fields:
            field['json_path'] = self.generate_json_path(field['field_name'])
        
        return all_fields
    
    def save_to_csv(self, fields: List[Dict[str, Any]], filename: str):
        """Save fields data to CSV file."""
        if not fields:
            print("No fields to save")
            return
        
        with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = ['field_name', 'json_path', 'required_quote', 'required_bind', 
                         'section', 'description']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            writer.writeheader()
            for field in fields:
                writer.writerow(field)
        
        print(f"Data saved to {filename}")
    
    def save_to_json(self, fields: List[Dict[str, Any]], filename: str):
        """Save fields data to JSON file."""
        with open(filename, 'w', encoding='utf-8') as jsonfile:
            json.dump(fields, jsonfile, indent=2, ensure_ascii=False)
        
        print(f"Data saved to {filename}")

def main():
    """Main function to run the scraper."""
    # Configuration
    base_url = "https://eigservices.atlassian.net"
    page_url = "https://eigservices.atlassian.net/wiki/spaces/DASAPI/pages/2063762529/Master+Field+List"
    
    # Initialize scraper (you may need to provide credentials)
    scraper = AtlassianFieldScraper(base_url)
    
    print("Scraping Master Field List page...")
    fields = scraper.scrape_all_sections(page_url)
    
    if fields:
        print(f"Found {len(fields)} fields")
        
        # Save to both CSV and JSON
        scraper.save_to_csv(fields, 'master_field_list.csv')
        scraper.save_to_json(fields, 'master_field_list.json')
        
        # Print sample data
        print("\nSample fields:")
        for i, field in enumerate(fields[:5]):
            print(f"{i+1}. {field['field_name']}")
            print(f"   JSON Path: {field['json_path']}")
            print(f"   Section: {field['section']}")
            print(f"   Description: {field['description']}")
            print()
    else:
        print("No fields found. This might be due to:")
        print("1. Authentication required")
        print("2. Page structure different than expected")
        print("3. Network access issues")
        print("\nYou may need to:")
        print("1. Provide credentials for the Atlassian instance")
        print("2. Manually copy the page content")
        print("3. Use the PDF you mentioned")

if __name__ == "__main__":
    main()