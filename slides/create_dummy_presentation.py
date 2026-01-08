import os
import subprocess

markdown_content = """---
title: "PostgreSQL: Advanced Open Source Database"
author: "Josef"
date: "2023-10-27"
theme: "Warsaw"
aspectratio: 169
---

# Introduction

## What is PostgreSQL?

![](postgres_logo.png){width=100px}

- **Object-Relational** Database System
- Open Source (PostgreSQL License)
- Known for reliability and feature robustness
- "The World's Most Advanced Open Source Relational Database"

::: notes
Speaker Note: Emphasize the community-driven nature and the BSD-style license which is very permissive.
:::

# Architecture

## Process Model

- **Postmaster**: Main daemon process
- **Background Workers**: Checkpointer, Writer, WalWriter, Autovacuum
- **Backend Processes**: One per client connection

![](architecture_diagram.png){width=250px}

::: notes
Speaker Note: Architecture is process-based, not thread-based. This provides stability but has memory overhead per connection.
Suggested Solution: PgBouncer.
:::

## Memory Architecture

- **Shared Buffers**: Database cache
- **WAL Buffers**: Write-Ahead Log
- **Work Mem**: Sort/Hash operations (per operation)
- **Maintenance Work Mem**: Vacuum/Index creation

# Key Features

## Performance Comparison

![](performance_graph.png){width=280px}

- PostgreSQL offers competitive throughput
- Effective handling of concurrent connections
- JSONB performance rivals NoSQL stores

## JSONB and NoSQL

- Binary JSON storage
- Indexing (GIN, GiST, B-Tree)
- Full standard SQL support alongside JSON
- Great for hybrid data models

::: notes
This is a huge differentiator from MySQL. JSONB allows for high-performance document storage within a relational engine.
:::

## Extensibility

- **PostGIS**: Geospatial data
- **TimescaleDB**: Time-series
- **pgvector**: Vector similarity files (AI)
- Custom Types and Functions

# Conclusion

## Summary

- Robust ACID compliance
- Rich feature set
- Extensible architecture
- Vibrant community

## Questions?
"""

def create_pdf(filename, split=False):
    md_file = "temp_pres.md"
    
    # Add header for split view if needed
    current_md = markdown_content
    if split:
        # Inject beamer options for split notes
        header_includes = """
header-includes:
 - \\usepackage{pgfpages}
 - \\setbeameroption{show notes on second screen=right}
"""
        # Insert after the first ---
        parts = current_md.split('---', 2)
        if len(parts) >= 3:
            current_md = f"---{parts[1]}{header_includes}---{parts[2]}"
    
    with open(md_file, "w") as f:
        f.write(current_md)
        
    cmd = ["pandoc", md_file, "-t", "beamer", "-o", filename]
    
    print(f"Generating {filename}...")
    try:
        subprocess.run(cmd, check=True)
        print("Success!")
    except subprocess.CalledProcessError as e:
        print(f"Error generating PDF: {e}")
    finally:
        if os.path.exists(md_file):
            os.remove(md_file)

if __name__ == "__main__":
    create_pdf("postgresql_presentation.pdf", split=False)
    create_pdf("postgresql_presentation_split.pdf", split=True)
