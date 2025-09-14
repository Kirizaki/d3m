#!/usr/bin/env python3
import yaml
import html

input_yaml = "fixes.yaml"
output_html = "clang-tidy-report.html"

with open(input_yaml) as f:
    data = yaml.safe_load(f)

diagnostics = data.get("Diagnostics", [])

html_content = f"""
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Clang-Tidy Report</title>
<style>
body {{ font-family: Arial, sans-serif; margin: 20px; background: #f9f9f9; }}
h1 {{ color: #2c3e50; }}
table {{ border-collapse: collapse; width: 100%; margin-bottom: 20px; }}
th, td {{ border: 1px solid #ccc; padding: 8px; text-align: left; }}
th {{ background-color: #2c3e50; color: white; }}
tr:nth-child(even) {{ background-color: #ecf0f1; }}
.code {{ font-family: monospace; background-color: #f0f0f0; padding: 2px 4px; border-radius: 3px; }}
</style>
</head>
<body>
<h1>Clang-Tidy Report</h1>
<p>Total issues: {len(diagnostics)}</p>
<table>
<tr><th>File</th><th>Line</th><th>Message</th><th>Check</th></tr>
"""

for diag in diagnostics:
    msg_info = diag.get("DiagnosticMessage", {})
    file_path = msg_info.get("FilePath", "N/A")
    line = msg_info.get("FileOffset", "N/A")
    message = msg_info.get("Message", "N/A")
    check = diag.get("DiagnosticName", "N/A")

    html_content += (
        f"<tr>"
        f"<td class='code'>{html.escape(file_path)}</td>"
        f"<td>{line}</td>"
        f"<td>{html.escape(message)}</td>"
        f"<td>{html.escape(check)}</td>"
        f"</tr>\n"
    )

html_content += "</table>\n</body>\n</html>"

with open(output_html, "w") as f:
    f.write(html_content)

print(f"HTML report generated: {output_html}")
