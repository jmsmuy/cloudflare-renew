#!/usr/bin/env python3
"""
Helper script to process field data from manually copied content or PDF.
This can help structure the data into the requested format.
"""

import csv
import json
import re
from typing import List, Dict, Any

class FieldDataProcessor:
    def __init__(self):
        self.fields = []
    
    def add_field(self, field_name: str, description: str, section: str, 
                  required_quote: bool = False, required_bind: bool = False):
        """Add a field to the collection."""
        json_path = self.generate_json_path(field_name)
        
        field_data = {
            'field_name': field_name,
            'json_path': json_path,
            'required_quote': required_quote,
            'required_bind': required_bind,
            'section': section,
            'description': description
        }
        
        self.fields.append(field_data)
        return field_data
    
    def generate_json_path(self, field_name: str) -> str:
        """Generate JSON path based on field name hierarchy."""
        # Split by common separators
        parts = re.split(r'\s*-\s*|\s*>\s*|\s*\.\s*', field_name)
        
        # Convert to camelCase and handle special cases
        json_parts = []
        for part in parts:
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
    
    def save_to_csv(self, filename: str = 'field_data.csv'):
        """Save fields data to CSV file."""
        if not self.fields:
            print("No fields to save")
            return
        
        with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = ['field_name', 'json_path', 'required_quote', 'required_bind', 
                         'section', 'description']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            writer.writeheader()
            for field in self.fields:
                writer.writerow(field)
        
        print(f"Data saved to {filename}")
    
    def save_to_json(self, filename: str = 'field_data.json'):
        """Save fields data to JSON file."""
        with open(filename, 'w', encoding='utf-8') as jsonfile:
            json.dump(self.fields, jsonfile, indent=2, ensure_ascii=False)
        
        print(f"Data saved to {filename}")
    
    def print_sample(self, count: int = 5):
        """Print sample fields."""
        print(f"\nSample fields ({min(count, len(self.fields))}):")
        for i, field in enumerate(self.fields[:count]):
            print(f"{i+1}. {field['field_name']}")
            print(f"   JSON Path: {field['json_path']}")
            print(f"   Section: {field['section']}")
            print(f"   Required Quote: {field['required_quote']}")
            print(f"   Required Bind: {field['required_bind']}")
            print(f"   Description: {field['description']}")
            print()

def example_usage():
    """Example of how to use the processor."""
    processor = FieldDataProcessor()
    
    # Example fields (you would replace these with actual data from the pages)
    processor.add_field(
        "Named Insureds - Locations - Address - Zip Code",
        "The postal code for the insured location",
        "Initial",
        required_quote=True,
        required_bind=True
    )
    
    processor.add_field(
        "Policy - Effective Date",
        "The date when the policy becomes effective",
        "Finalize",
        required_quote=True,
        required_bind=True
    )
    
    processor.add_field(
        "Coverage - Limits - General Liability",
        "The maximum amount covered for general liability",
        "Bind",
        required_quote=False,
        required_bind=True
    )
    
    # Save and display results
    processor.save_to_csv()
    processor.save_to_json()
    processor.print_sample()

if __name__ == "__main__":
    example_usage()