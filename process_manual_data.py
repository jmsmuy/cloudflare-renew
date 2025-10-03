#!/usr/bin/env python3
"""
Script to help process manually copied data from Atlassian wiki or PDF.
This script provides an interactive way to enter field data.
"""

import csv
import json
from field_data_processor import FieldDataProcessor

def interactive_data_entry():
    """Interactive data entry for field information."""
    processor = FieldDataProcessor()
    
    print("=== Atlassian Field Data Entry Tool ===")
    print("Enter field data from the Initial, Finalize, and Bind sections.")
    print("Press Enter with empty field name to finish.\n")
    
    sections = ['Initial', 'Finalize', 'Bind']
    
    for section in sections:
        print(f"\n--- {section} Quote Fields ---")
        print("Enter fields for this section (press Enter with empty name to move to next section):")
        
        while True:
            field_name = input("\nField Name (hierarchical, e.g., 'Named Insureds - Locations - Address - Zip Code'): ").strip()
            if not field_name:
                break
            
            description = input("Description: ").strip()
            
            required_quote_input = input("Required for Quote? (y/n): ").strip().lower()
            required_quote = required_quote_input in ['y', 'yes', 'true', '1']
            
            required_bind_input = input("Required for Bind? (y/n): ").strip().lower()
            required_bind = required_bind_input in ['y', 'yes', 'true', '1']
            
            # Add the field
            field_data = processor.add_field(
                field_name=field_name,
                description=description,
                section=section,
                required_quote=required_quote,
                required_bind=required_bind
            )
            
            print(f"Added: {field_data['field_name']}")
            print(f"JSON Path: {field_data['json_path']}")
    
    # Save the data
    if processor.fields:
        processor.save_to_csv('manual_field_data.csv')
        processor.save_to_json('manual_field_data.json')
        processor.print_sample()
        print(f"\nTotal fields processed: {len(processor.fields)}")
    else:
        print("No fields were entered.")

def batch_data_entry():
    """Batch data entry using predefined data structure."""
    processor = FieldDataProcessor()
    
    # Example data structure - you can modify this with your actual data
    sample_data = [
        {
            'field_name': 'Named Insureds - Locations - Address - Zip Code',
            'description': 'The postal code for the insured location',
            'section': 'Initial',
            'required_quote': True,
            'required_bind': True
        },
        {
            'field_name': 'Named Insureds - Locations - Address - Street Address',
            'description': 'The street address of the insured location',
            'section': 'Initial',
            'required_quote': True,
            'required_bind': True
        },
        {
            'field_name': 'Policy - Effective Date',
            'description': 'The date when the policy becomes effective',
            'section': 'Finalize',
            'required_quote': True,
            'required_bind': True
        },
        {
            'field_name': 'Coverage - Limits - General Liability',
            'description': 'The maximum amount covered for general liability claims',
            'section': 'Bind',
            'required_quote': False,
            'required_bind': True
        }
    ]
    
    print("=== Batch Data Entry ===")
    print("Processing sample data...")
    
    for data in sample_data:
        processor.add_field(**data)
    
    processor.save_to_csv('batch_field_data.csv')
    processor.save_to_json('batch_field_data.json')
    processor.print_sample()
    
    print(f"\nTotal fields processed: {len(processor.fields)}")
    print("\nTo use your own data, modify the 'sample_data' list in this script.")

def main():
    """Main function with menu options."""
    print("Choose data entry method:")
    print("1. Interactive entry (recommended for manual data)")
    print("2. Batch entry (modify script with your data)")
    print("3. Exit")
    
    choice = input("\nEnter your choice (1-3): ").strip()
    
    if choice == '1':
        interactive_data_entry()
    elif choice == '2':
        batch_data_entry()
    elif choice == '3':
        print("Goodbye!")
    else:
        print("Invalid choice. Please run the script again.")

if __name__ == "__main__":
    main()