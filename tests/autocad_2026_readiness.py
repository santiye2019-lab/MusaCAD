#!/usr/bin/env python3
"""Evidence-backed AutoCAD 2026 acceptance readiness, not a parity certificate.

A passing CI build is not enough. This report distinguishes actual completed
reference/device gates from partially implemented or purely listed commands.
``--require-ready`` fails unless every verified family AND the command
inventory have independently completed evidence. It is a FINAL release gate,
NOT a prerequisite for generating the first user-approved candidate APK,
because a physical device check needs an installable candidate to exist.
"""
import argparse
import json

from autocad_2026_51_acceptance import GATES, LEDGER, MATRIX, validate_ledger


def summarize(matrix, ledger):
    certified = validate_ledger(matrix, ledger)
    families = []
    gate_passes = {gate: 0 for gate in GATES}
    scenario_passes = 0
    scenario_total = 0
    unfinished_inventories = 0
    for case in ledger["cases"]:
        scenarios = case["scenarios"]
        scenario_total += len(scenarios)
        scenario_passes += sum(s["status"] == "passed" for s in scenarios)
        if not case["scenario_inventory_complete"]:
            unfinished_inventories += 1
        missing_gates = [
            name for name in GATES
            if case["gates"][name]["status"] != "passed"
        ]
        for name in GATES:
            gate_passes[name] += case["gates"][name]["status"] == "passed"
        families.append({
            "id": case["id"],
            "capability": case["capability"],
            "scenario_inventory_complete": case["scenario_inventory_complete"],
            "scenarios_passed": sum(s["status"] == "passed" for s in scenarios),
            "scenarios_defined": len(scenarios),
            "missing_gates": missing_gates,
            "review": case["review"]["decision"],
        })
    complete_catalog = (
        matrix.get("scoring_policy", {}).get("complete_catalog") is True
    )
    total = certified["total"]
    fully_ready = (
        certified["approved"] == total
        and unfinished_inventories == 0
        and complete_catalog
    )
    return {
        "schema_version": 1,
        "product": "AutoCAD 2026",
        "readiness": "ready" if fully_ready else "blocked",
        "certified_families": certified["approved"],
        "family_count": total,
        "scenario_inventories_incomplete": unfinished_inventories,
        "scenarios_defined": scenario_total,
        "scenarios_passed": scenario_passes,
        "reference_command_inventory_complete": complete_catalog,
        "gate_pass_counts": gate_passes,
        "families": families,
        "notice": (
            "This is an audit of the preliminary 51-family inventory, NOT "
            "proof that the entire AutoCAD 2026 command catalogue is covered."
        ),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true",
                        help="Machine-readable missing evidence report")
    parser.add_argument("--require-ready", action="store_true",
                        help="Fail CLOSED until independently accepted final parity")
    options = parser.parse_args()
    matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
    ledger = json.loads(LEDGER.read_text(encoding="utf-8"))
    report = summarize(matrix, ledger)
    if options.json:
        print(json.dumps(report, ensure_ascii=False, indent=2))
    else:
        print("AutoCAD 2026 independent acceptance: "
              "{certified_families}/{family_count} verified; "
              "{scenario_inventories_incomplete} family scenario inventories "
              "incomplete; reference command catalogue complete: "
              "{reference_command_inventory_complete}".format(**report))
        print("Verified gate counts (out of {} families):".format(
            report["family_count"]))
        for gate in GATES:
            print("  {}: {}".format(gate, report["gate_pass_counts"][gate]))
        print("Final parity readiness: " + report["readiness"].upper())
    return 0 if not options.require_ready or report["readiness"] == "ready" else 1


if __name__ == "__main__":
    raise SystemExit(main())
