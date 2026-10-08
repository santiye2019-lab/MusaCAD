const MAX_BODY_BYTES = 4 * 1024 * 1024;
const MAX_PROMPT_CHARS = 12000;
const OPENAI_RESPONSES_URL = "https://api.openai.com/v1/responses";
const GEMINI_CHAT_COMPLETIONS_URL = "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions";

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === "/health") return json({ status: "ok", service: "musacad-ai-worker" });
    if (url.pathname === "/v1/analyze") return handleAnalyze(request, env);
    return json({ status: "not_found" }, 404);
  }
};

async function handleAnalyze(request, env) {
  if (request.method !== "POST") return json({ status: "method_not_allowed" }, 405, { Allow: "POST" });
  const aiProvider = selectAiProvider(env);
  if (!aiProvider.ok || !env.MUSACAD_AI_SESSION_PUBLIC_KEY_PEM)
    return json({ status: "server_error", message: aiProvider.message || "AI worker is not fully configured" }, 503);

  const session = await verifySession(request.headers.get("authorization"), env.MUSACAD_AI_SESSION_PUBLIC_KEY_PEM);
  if (!session.ok) return json({ status: "denied", message: session.message }, 401);

  const parsed = await readJson(request);
  if (!parsed.ok) return parsed.response;
  const body = parsed.body || {};
  const prompt = String(body.prompt || "").trim();
  const cad = body.cad;
  const cadPackage = body.cadPackage;
  const packageMode = cadPackage !== undefined && cadPackage !== null;
  const allowWeb = body.allowWeb === true;
  const allowEditProposals = body.allowEditProposals === true;
  const expertProfile = normalizeExpertProfile(body.expertProfile);
  const allowedScopes = new Set(["all","mechanical","sanitary","wastewater","fire","ventilation",
    "heating","cooling","gas","structural","architectural","electrical","landscape","infrastructure","elevator"]);
  const analysisScope = typeof body.analysisScope === "string" && allowedScopes.has(body.analysisScope)
    ? body.analysisScope : "all";
  const visualEvidence = body.visualEvidence == null ? null : validateVisualEvidence(body.visualEvidence);
  if (body.visualEvidence != null && !visualEvidence)
    return json({ status: "denied", message: "Invalid or oversized visual CAD evidence" }, 400);

  if (!prompt || prompt.length > MAX_PROMPT_CHARS)
    return json({ status: "denied", message: "Prompt is empty or too large" }, 400);
  if (!cad || typeof cad !== "object" || cad.schema !== "musacad-cad-json/v1")
    return json({ status: "denied", message: "Unsupported CAD-JSON payload" }, 400);
  if (cad.cloudPolicy && cad.cloudPolicy.rawDrawingIncluded === true)
    return json({ status: "denied", message: "Raw drawing upload is not accepted by this endpoint" }, 400);
  if (packageMode && !validCadPackage(cadPackage))
    return json({ status: "denied", message: "Unsupported CAD package payload" }, 400);

  const tools = [];
  if (allowWeb) tools.push({ type: "web_search" });
  // Package mode is deliberately read-only because sourceIds are local to each drawing.
  if (allowEditProposals && !packageMode) tools.push(...cadProposalTools());

  const accessMode = session.mode === "developer" ? "developer" : "licensed";
  const instructions =
    "You are Gandalf AI inside MusaCAD, an engineering CAD assistant. " +
    "Current access mode: " + accessMode + ". " +
    (accessMode === "developer"
      ? "Developer mode may use the full bounded analysis and proposal surface, but drawing edits still require explicit user approval. "
      : "") +
    "Analyze the supplied bounded CAD-JSON across architectural, structural, mechanical, electrical, landscape, infrastructure, elevator and fire-safety systems when present. " +
    "User-requested primary discipline scope: " + analysisScope + ". Emphasize this system's engineering constraints; use other disciplines only for coordination. For all, identify relevant disciplines from evidence rather than inventing discipline-specific findings. " +
    (visualEvidence
      ? "The user explicitly consented to visual CAD evidence. Combine visual sheet images with vector annotations/measurements. Visual images cover labeled sheet regions; they may be incomplete or low resolution. Identify lines as pipes/ducts/equipment ONLY with legend, shape, topology, annotation or other supporting evidence, and mark uncertain classification as a candidate. Distinguish missing-from-this-image from missing-from-project. Do not imply any unchecked part of the plan was reviewed. Do not invent equipment Q/H ratings, pipe diameter, duct size or reliable quantity takeoff without readable corroborating data. "
      : "No visual images were supplied. Do not claim you visually examined the DWG sheet; only CAD metadata is available. ") +
    (packageMode
      ? "A bounded MusaCAD CAD package containing multiple open drawings is also supplied. Treat each drawing as a separate source, compare disciplines explicitly, use fileName and detectedDiscipline to attribute findings, and distinguish cross-drawing proximity/coordination candidates from proven clashes. Package mode is read-only: do not claim or propose CAD edits across files. "
      : "") +
    mechanicalExpertInstructions(expertProfile) +
    disciplineExpertInstructions(expertProfile) +
    "If visualEvidence contains a sweepBatch, analyze ONLY those named regions and the overview; never imply that other high-resolution regions were scanned. State each finding with its visible region label, observed component, supporting drawing annotation/geometry when present, degree of confidence, and what needs verification. Separate confirmed visible observations, plausible candidates, and unverified design checks. A visual match to a CAD sourceId requires actual spatial and textual/graph evidence, not proximity alone. " +
    "Separate observations from assumptions and recommendations. Never claim a drawing is code-compliant, safe, or approved merely from this data. " +
    "Call out missing information and confidence limits. " +
    "If edit tools are available, tool calls are PROPOSALS ONLY. They are not executed automatically and require explicit user approval in MusaCAD. " +
    "For cad_change_layer, cad_add_line, cad_add_text, cad_add_polyline, cad_add_pipe_note and cad_insert_mechanical_block, use an exact existing layer name visible in the supplied CAD-JSON; never invent a new layer name. " +
    "Editable LINE/POLYLINE items may include vertices in drawing units. Use only supplied vertices/sourceIds and explicit user coordinates; never invent geometry coordinates. " +
    "TRIM and EXTEND intersections are calculated by MusaCAD, not by you: identify the correct target/boundary sourceIds and, for TRIM, provide a pick point near the side that should be removed. " +
    "Never state that a proposed edit has already been applied. " +
    "Prefer sourceId-based edits for existing entities. Use web search only when it materially helps the user's request, and identify external sources in the answer.";

  const input = [
    {
      role: "user",
      content:
        "USER REQUEST:\n" + prompt +
        "\n\nMUSACAD CAD-JSON (active drawing):\n" + JSON.stringify(cad) +
        (packageMode ? "\n\nMUSACAD CAD-PACKAGE:\n" + JSON.stringify(cadPackage) : "")
    }
  ];

  if (visualEvidence) {
    input[0].content += "\n\nVISUAL SWEEP BATCH: " + (visualEvidence.sweepBatch ?? "legacy") +
      " / " + (visualEvidence.sweepBatch ? 3 : 1) +
      ". Do not claim all nine tiles were checked based on this single request." +
      "\n\nVISUAL CAD REGIONS: " + JSON.stringify(visualEvidence.regions) +
      "\nEvery image is a separate region of the same loaded DWG. contentBounds are rendered viewport-content coordinates, while drawingBounds are DWG world coordinates shared with vector item.centerX/centerY. Use drawingBounds when correlating a visual finding with CAD items; never claim a sourceId match from proximity alone.";
  }

  const maxOutputTokens = positiveInt(env.AI_MAX_OUTPUT_TOKENS || env.OPENAI_MAX_OUTPUT_TOKENS, 3200, 512, 12000);
  let requestBody;
  let upstreamUrl;
  let upstreamKey;

  if (aiProvider.name === "gemini") {
    const geminiTools = tools
      .filter(tool => tool && tool.type === "function")
      .map(toChatCompletionsTool);
    let systemInstructions = instructions;
    if (allowWeb) {
      systemInstructions += " This Gemini Free Tier route has no live web-search tool. Do not claim that you searched the web or verified current sources; state that current web verification is unavailable when relevant.";
    }
    requestBody = {
      model: aiProvider.model,
      messages: [
        { role: "system", content: systemInstructions },
        { role: "user", content: visualEvidence
          ? [{ type: "text", text: input[0].content }, ...visualEvidence.images.map(
              frame => ({ type: "image_url", image_url: { url: "data:image/jpeg;base64," + frame.base64 } }))]
          : input[0].content }
      ],
      max_tokens: maxOutputTokens
    };
    if (geminiTools.length) {
      requestBody.tools = geminiTools;
      requestBody.tool_choice = "auto";
    }
    upstreamUrl = GEMINI_CHAT_COMPLETIONS_URL;
    upstreamKey = String(env.GEMINI_API_KEY);
  } else {
    requestBody = {
      model: aiProvider.model,
      instructions,
      input: visualEvidence ? [{
        role: "user",
        content: [
          { type: "input_text", text: input[0].content },
          ...visualEvidence.images.map(frame => ({
            type: "input_image", image_url: "data:image/jpeg;base64," + frame.base64
          }))
        ]
      }] : input,
      tools,
      parallel_tool_calls: true,
      store: false,
      max_output_tokens: maxOutputTokens
    };
    if (allowWeb) requestBody.include = ["web_search_call.action.sources"];
    upstreamUrl = OPENAI_RESPONSES_URL;
    upstreamKey = String(env.OPENAI_API_KEY);
  }

  const fetcher = typeof env.__fetch === "function" ? env.__fetch : fetch;
  const upstreamTimeoutMs = positiveInt(env.AI_UPSTREAM_TIMEOUT_MS, 24000, 50, 25000);
  const controller = new AbortController();
  let timeoutId;
  const deadline = new Promise((_, reject) => {
    timeoutId = setTimeout(() => {
      controller.abort();
      reject(new Error("AI_UPSTREAM_TIMEOUT"));
    }, upstreamTimeoutMs);
  });
  let upstream, data;
  try {
    try {
      upstream = await Promise.race([fetcher(upstreamUrl, {
        method: "POST",
        headers: {
          authorization: "Bearer " + upstreamKey,
          "content-type": "application/json",
          accept: "application/json"
        },
        body: JSON.stringify(requestBody),
        signal: controller.signal
      }), deadline]);
    } catch (_) {
      if (controller.signal.aborted)
        return json({ status: "timeout", message: "Gandalf AI modeli süresi içinde yanıt vermedi. Yerel proje analizi kullanılabilir." }, 504);
      return json({ status: "server_error", message: aiProvider.name === "gemini" ? "Gemini connection failed" : "OpenAI connection failed" }, 503);
    }

    try {
      data = await Promise.race([upstream.json(), deadline]);
    } catch (_) {
      if (controller.signal.aborted)
        return json({ status: "timeout", message: "Gandalf AI yanıtı gecikti. Yerel proje analizi kullanılabilir." }, 504);
      return json({ status: "server_error", message: aiProvider.name === "gemini" ? "Gemini returned invalid JSON" : "OpenAI returned invalid JSON" }, 502);
    }
  } finally {
    clearTimeout(timeoutId);
  }
  if (!upstream.ok) {
    if (upstream.status === 429 && aiProvider.name === "gemini") {
      return json({
        status: "quota_exhausted",
        message: "Gemini ücretsiz kullanım kotası şu anda dolu. Kota yenilendiğinde Gandalf otomatik olarak yeniden çalışır."
      }, 429);
    }
    const upstreamMessage = data && data.error && data.error.message
      ? String(data.error.message)
      : (aiProvider.name === "gemini" ? "Gemini request failed" : "OpenAI request failed");
    return json({ status: "server_error", message: upstreamMessage.slice(0, 300) }, upstream.status >= 500 ? 503 : 502);
  }

  const parsedOutput = aiProvider.name === "gemini" ? parseGeminiChatOutput(data) : parseOpenAiOutput(data);
  const reply = parsedOutput.reply || (parsedOutput.actions.length
    ? "Gandalf AI çizim için " + parsedOutput.actions.length + " adet düzenleme önerisi hazırladı. Bu işlemler henüz uygulanmadı."
    : "Gandalf AI yanıt üretemedi.");

  return json({
    status: "ok",
    reply,
    actions: parsedOutput.actions,
    sources: parsedOutput.sources,
    webUsed: parsedOutput.webUsed,
    sessionExpiresAtMs: session.expiresAtMs,
    accessMode,
    expertProfile,
    packageMode,
    provider: aiProvider.name,
    model: aiProvider.model,
    analysisScope,
    visualRegionCount: visualEvidence ? visualEvidence.images.length : 0,
    visualCoverageComplete: visualEvidence ? visualEvidence.complete : false,
    usage: parsedOutput.usage
  });
}

function validateVisualEvidence(value) {
  if (!value || typeof value !== "object" ||
      value.schema !== "musacad-visual-evidence/v1" ||
      value.rawDrawingIncluded !== false || !Array.isArray(value.images) ||
      value.images.length < 1 || value.images.length > 5) return null;

  const sweep = value.sweepSchema === "musacad-visual-sweep/v1";
  if (value.sweepSchema != null && !sweep) return null;
  let first = 0, last = 0;
  if (sweep) {
    if (value.sweepBatchCount !== 3 || value.totalDetailedTiles !== 9 ||
        !Number.isInteger(value.sweepBatch) || value.sweepBatch < 1 || value.sweepBatch > 3 ||
        !Number.isInteger(value.firstTile) || !Number.isInteger(value.lastTile)) return null;
    first = (value.sweepBatch - 1) * 4 + 1;
    last = Math.min(9,first+3);
    if (value.firstTile !== first || value.lastTile !== last) return null;
  }

  let total = 0;
  const images = [], regions = [];
  const labels = new Set();
  for (let i=0;i<value.images.length;i++) {
    const img = value.images[i];
    if (!img || img.mime !== "image/jpeg" || typeof img.base64 !== "string" ||
        img.base64.length < 100 || img.base64.length > 900000 ||
        !/^\/9j\/[A-Za-z0-9+/]*={0,2}$/.test(img.base64) ||
        typeof img.label !== "string" ||
        !(sweep ? /^(full-sheet-overview|sheet-tile-[1-9])$/.test(img.label)
                 : /^(full-sheet-overview|sheet-quadrant-[1-4])$/.test(img.label)) ||
        labels.has(img.label) ||
        !Number.isInteger(img.width) || img.width < 128 || img.width > 1200 ||
        !Number.isInteger(img.height) || img.height < 128 || img.height > 1200 ||
        !Array.isArray(img.contentBounds) || img.contentBounds.length !== 4 ||
        !img.contentBounds.every(n => typeof n === "number" && Number.isFinite(n)) ||
        img.contentBounds[2] <= img.contentBounds[0] ||
        img.contentBounds[3] <= img.contentBounds[1] ||
        !Array.isArray(img.drawingBounds) || img.drawingBounds.length !== 4 ||
        !img.drawingBounds.every(n => typeof n === "number" && Number.isFinite(n)) ||
        img.drawingBounds[2] <= img.drawingBounds[0] ||
        img.drawingBounds[3] <= img.drawingBounds[1]) return null;
    if (i === 0 && img.label !== "full-sheet-overview") return null;
    if (sweep && i>0 && img.label !== "sheet-tile-"+(first+i-1)) return null;
    labels.add(img.label);
    total += img.base64.length;
    if (total > 1650000) return null;
    images.push({ base64: img.base64 });
    regions.push({ label: img.label, contentBounds: img.contentBounds,
      drawingBounds: img.drawingBounds, width: img.width, height: img.height });
  }
  if (sweep && value.images.length>last-first+2) return null;
  const allRequested = sweep
    ? images.length === last-first+2
    : images.length === 5;
  if (sweep && value.complete === true && !allRequested) return null;
  return {
    images, regions,
    complete: value.complete === true && allRequested,
    sweepBatch: sweep ? value.sweepBatch : null,
    totalDetailedTiles: sweep ? 9 : null
  };
}

function validCadPackage(value) {
  if (!value || typeof value !== "object" || value.schema !== "musacad-cad-package/v1") return false;
  if (!Array.isArray(value.drawings) || value.drawings.length < 1 || value.drawings.length > 4) return false;
  if (value.cloudPolicy && value.cloudPolicy.rawDrawingIncluded === true) return false;
  if (value.cloudPolicy && value.cloudPolicy.packageEditToolsAllowed === true) return false;
  for (const drawing of value.drawings) {
    if (!drawing || typeof drawing !== "object") return false;
    const one = drawing.cad;
    if (!one || typeof one !== "object" || one.schema !== "musacad-cad-json/v1") return false;
    if (one.cloudPolicy && one.cloudPolicy.rawDrawingIncluded === true) return false;
    if (typeof drawing.fileName !== "string" || drawing.fileName.length > 160) return false;
  }
  return true;
}

function normalizeExpertProfile(value) {
  const known = new Set([
    "mechanical_full","waste","rain","water","heating",
    "cooling","ventilation","fire","gas","equipment",
    "arch_full","arch_access","arch_escape","arch_space","arch_envelope",
    "structural_full","structural_frame","structural_foundation","structural_openings","structural_stairs",
    "electrical_full","electrical_power","electrical_lighting","electrical_weak","electrical_grounding","electrical_emergency",
    "landscape_full","landscape_hard","landscape_soft","landscape_irrigation","landscape_drainage",
    "infrastructure_full","infrastructure_waste","infrastructure_rain","infrastructure_water","infrastructure_utilities","infrastructure_levels",
    "elevator_full","elevator_shaft","elevator_door","elevator_machine","elevator_electrical","elevator_fire",
    "fire_safety_full","fire_escape","fire_sprinkler","fire_hydrant","fire_detection","fire_smoke","fire_pump"
  ]);
  const key = String(value || "").trim().toLowerCase();
  return known.has(key) ? key : "";
}

function mechanicalExpertInstructions(profile) {
  if (!profile) return "";
  const common =
    "For this MEKAI expert request, structure the response as: Uzman Özeti, Tespitler, Eksik/Doğrulanamayan Veriler, and Önerilen Düzeltmeler. " +
    "For each concrete finding, identify sourceId and layer when available. Distinguish a directly observed CAD fact from an engineering inference. " +
    "Only propose a CAD tool action when the target sourceId or drawing coordinates are unambiguous in the supplied CAD-JSON. ";
  switch (profile) {
    case "mechanical_full":
      return common + "MEKAI expert profile: review all visible mechanical systems separately: waste, rainwater, domestic water, heating, cooling/VRF, ventilation, fire protection, natural gas, and mechanical equipment. Check system identification, labels, topology, cross-system coordination candidates, equipment references and missing design metadata. ";
    case "waste":
      return common + "MEKAI waste-water expert profile: focus on gravity/pumped waste distinction, visible pipe diameters, slopes, vents, cleanouts/manholes, fixture/branch continuity and suspicious open or degenerate runs. Do not infer slope or invert levels that are not present. ";
    case "rain":
      return common + "MEKAI rainwater expert profile: focus on roof drains, gutters, downpipes, pipe diameters, drainage continuity, overflow/emergency drainage references and visible slope/level information. ";
    case "water":
      return common + "MEKAI domestic-water expert profile: focus on cold/hot/return separation, diameters, isolation/control valves, meters, pressure reducing devices, tank/hydrofor/pump references and branch continuity. ";
    case "heating":
      return common + "MEKAI heating expert profile: focus on supply/return separation, diameters, pumps, collectors, heat source/exchanger references, balancing/control valves and zoning. Do not perform heat-load or pump sizing unless the required inputs are explicitly supplied. ";
    case "cooling":
      return common + "MEKAI cooling/VRF expert profile: focus on VRF/VRV/chiller/fan-coil system identification, indoor/outdoor units, refrigerant piping/branch/refnet references, condensate drainage, equipment capacities and visible connection continuity. ";
    case "ventilation":
      return common + "MEKAI ventilation expert profile: focus on duct dimensions, airflow labels, supply/return/exhaust/fresh-air identification, grilles/diffusers, fans/AHUs, dampers and fire/smoke damper candidates at relevant transitions. Do not infer airflow or pressure losses when absent. ";
    case "fire":
      return common + "MEKAI fire-protection expert profile: focus on sprinkler/hydrant/fire-cabinet systems, visible pipe diameters, pumps/jockey/tank, zone/alarm valves, test-and-drain references and fire-department connection. Treat code compliance as unverified unless current authoritative sources and all required project inputs are available. ";
    case "gas":
      return common + "MEKAI natural-gas expert profile: focus on visible pipe diameters, meter/regulator, shutoff/solenoid valves, detector references and appliance connections. Do not certify gas safety or code compliance from CAD metadata alone. ";
    case "equipment":
      return common + "MEKAI mechanical-equipment expert profile: focus on equipment tags, capacities, flow/pressure/power values, duty/standby references and visible piping/duct connections. Service clearances and maintainability require geometry/detail verification. ";
    default:
      return common;
  }
}

function disciplineExpertInstructions(profile) {
  if (!profile || ["mechanical_full","waste","rain","water","heating","cooling","ventilation","fire","gas","equipment"].includes(profile)) return "";
  const common =
    "For this MusaCAD discipline-expert request, structure the response as: Uzman Özeti, Tespitler, Eksik/Doğrulanamayan Veriler, Proje–Keşif Etkisi, and Önerilen Düzeltmeler. " +
    "For each concrete finding identify sourceId and layer when available. Separate directly observed CAD facts from engineering inference. " +
    "Do not certify structural safety, electrical safety, fire-code compliance, elevator conformity, accessibility compliance or statutory approval from bounded CAD metadata alone. " +
    "Only propose CAD edits when sourceId or drawing coordinates are unambiguous. ";
  const map = {
    arch_full:"MIMAI architectural full profile: review rooms/spaces, doors and clear passages, circulation, accessibility references, escape interfaces, shafts, levels/dimensions, façade and roof coordination, and cross-discipline clashes.",
    arch_access:"MIMAI accessibility profile: review visible ramps, accessible routes, door/clear-passage references, level transitions, accessible WC/parking references and continuity. Do not infer compliant slopes or dimensions when absent.",
    arch_escape:"MIMAI escape/circulation profile: review visible exits, corridors, stairs, fire doors, travel-path continuity and interfaces with fire-safety drawings. Treat legal egress compliance as unverified unless all required inputs and current authoritative sources are available.",
    arch_space:"MIMAI room/door profile: review room tags, door references, clear openings, shafts and function-to-space consistency candidates.",
    arch_envelope:"MIMAI envelope profile: review façade/roof references, openings, insulation/waterproofing notes, drainage interfaces and structural/mechanical penetrations.",
    structural_full:"STATIKAI structural full profile: review columns, beams, walls, slabs, foundations, stairs/shafts, continuity, openings/reservations and coordination with architectural/MEP drawings. Never conclude structural adequacy without calculation model/report and design inputs.",
    structural_frame:"STATIKAI frame profile: focus on columns, beams, walls, slabs, axis/tag consistency, continuity and suspicious missing/duplicate elements.",
    structural_foundation:"STATIKAI foundation profile: focus on raft/footing/pile references, foundation beams, pits/shafts and utility penetration coordination. Do not infer soil capacity.",
    structural_openings:"STATIKAI opening/reservation profile: focus on holes, sleeves, shafts and penetrations near structural elements. Flag uncoordinated penetrations and never recommend field drilling without structural approval.",
    structural_stairs:"STATIKAI stair/elevator profile: focus on stairs, landings, elevator shafts/pits, openings and architectural coordination.",
    electrical_full:"ELKAI electrical full profile: review power, panels, cable routes/trays, lighting, receptacles, weak-current systems, grounding/lightning protection, generator/UPS/emergency power and MEP equipment feeds.",
    electrical_power:"ELKAI power profile: focus on panels, feeders, cable/tray/busbar routes, equipment feeds and visible load/circuit labels. Do not infer cable sizing or protection coordination without calculations.",
    electrical_lighting:"ELKAI lighting profile: focus on luminaire layout/tags, switching/control references, emergency lighting interfaces and room coordination. Do not infer lux compliance without photometric inputs.",
    electrical_weak:"ELKAI weak-current profile: focus on data, CCTV, access control, telephone, fire-alarm interfaces and route/room coordination.",
    electrical_grounding:"ELKAI grounding/lightning profile: focus on grounding, bonding, earth electrodes, lightning protection and equipment bonding references. Do not certify electrical safety.",
    electrical_emergency:"ELKAI emergency-power profile: focus on generator, UPS, ATS/emergency panels, critical loads and fire/life-safety equipment feeds.",
    landscape_full:"PEYAI landscape full profile: review hardscape, softscape, planting, irrigation, drainage, lighting/accessibility references and underground-utility coordination.",
    landscape_hard:"PEYAI hardscape profile: focus on paving, curbs, pedestrian routes, ramps, levels and drainage interfaces.",
    landscape_soft:"PEYAI softscape profile: focus on trees/plants/lawns, planting zones and conflicts with utilities, structures and maintenance access.",
    landscape_irrigation:"PEYAI irrigation profile: focus on irrigation lines/zones/valves and conflicts with planting, hardscape and utilities. Do not infer hydraulic adequacy without inputs.",
    landscape_drainage:"PEYAI drainage profile: focus on surface drainage, gullies, slopes/levels and connections to stormwater infrastructure.",
    infrastructure_full:"ALTYAPIAI infrastructure full profile: review wastewater, stormwater, water, gas/energy/telecom utilities, manholes, levels/slopes, crossings and authority connection points.",
    infrastructure_waste:"ALTYAPIAI wastewater profile: focus on sewers, manholes, slopes/invert-level references, connections and crossings. Do not infer invert levels when absent.",
    infrastructure_rain:"ALTYAPIAI stormwater profile: focus on storm drains, manholes/inlets, slopes/levels, discharge points and landscape/roof drainage interfaces.",
    infrastructure_water:"ALTYAPIAI water profile: focus on mains, branches, valves, chambers/meters and authority connection references. Do not certify pressure/flow adequacy without calculations.",
    infrastructure_utilities:"ALTYAPIAI utilities profile: focus on electrical, telecom and gas utility corridors, crossings, separation candidates and authority connection points.",
    infrastructure_levels:"ALTYAPIAI level profile: focus on visible levels, slopes, manholes, start/end elevations and crossing coordination.",
    elevator_full:"ASNAI elevator full profile: review shaft, pit, overhead, doors, machine/drive references, electrical/control interfaces, ventilation and fire scenario coordination. Do not certify EN/TS or statutory conformity from CAD metadata alone.",
    elevator_shaft:"ASNAI shaft profile: focus on shaft dimensions/references, pit, overhead, structural openings and architectural alignment.",
    elevator_door:"ASNAI door/access profile: focus on landing doors, clear openings, access and architectural/fire-door coordination.",
    elevator_machine:"ASNAI machine/drive profile: focus on machine/drive/control-room references, maintenance access and structural/electrical interfaces.",
    elevator_electrical:"ASNAI electrical profile: focus on supply, control panel, grounding, emergency/backup references and fire-safety interfaces.",
    elevator_fire:"ASNAI fire profile: focus on fire recall/firefighter operation references, lobby/door interfaces and emergency-power coordination.",
    fire_safety_full:"YANGAI fire/life-safety full profile: review escape, fire doors, sprinkler/hydrant/fire-cabinet systems, detection/alarm, smoke control/pressurization, pumps/tanks and fire-department access/connection. Treat code compliance as unverified without complete inputs and current authoritative sources.",
    fire_escape:"YANGAI escape profile: focus on exits, corridors, stairs, fire doors and continuity between architectural and fire-safety drawings.",
    fire_sprinkler:"YANGAI sprinkler profile: focus on sprinkler references, piping/zone/test-drain interfaces and coordination with ceilings/structure. Do not infer hydraulic adequacy.",
    fire_hydrant:"YANGAI hydrant profile: focus on hydrants, fire cabinets, fire-department connection and visible routing/zone interfaces.",
    fire_detection:"YANGAI detection profile: focus on detectors, manual call points, sounders, panels, loops/zones and electrical/architectural coordination.",
    fire_smoke:"YANGAI smoke-control profile: focus on smoke exhaust, pressurization, dampers, fans and architectural/mechanical interfaces. Do not infer smoke-control performance without calculations.",
    fire_pump:"YANGAI pump/tank profile: focus on fire pumps, jockey pump, tank, test/drain and electrical/emergency-power interfaces."
  };
  const specific = map[profile];
  return specific ? common + specific + " " : common;
}

function cadProposalTools() {
  return [
    functionTool(
      "cad_highlight_entities",
      "Propose highlighting existing CAD entities for user review. Does not modify the drawing.",
      {
        sourceIds: { type: "array", items: { type: "integer" }, minItems: 1, maxItems: 200 },
        reason: { type: "string" }
      },
      ["sourceIds", "reason"]
    ),
    functionTool(
      "cad_move_entity",
      "Propose moving one existing entity by a drawing-unit delta.",
      {
        sourceId: { type: "integer" },
        dx: { type: "number" },
        dy: { type: "number" },
        reason: { type: "string" }
      },
      ["sourceId", "dx", "dy", "reason"]
    ),
    functionTool(
      "cad_delete_entity",
      "Propose deleting one existing CAD entity.",
      {
        sourceId: { type: "integer" },
        reason: { type: "string" }
      },
      ["sourceId", "reason"]
    ),
    functionTool(
      "cad_change_layer",
      "Propose moving one existing CAD entity to another layer.",
      {
        sourceId: { type: "integer" },
        layer: { type: "string" },
        reason: { type: "string" }
      },
      ["sourceId", "layer", "reason"]
    ),
    functionTool(
      "cad_add_line",
      "Propose adding a line using active drawing coordinates.",
      {
        x1: { type: "number" }, y1: { type: "number" },
        x2: { type: "number" }, y2: { type: "number" },
        layer: { type: "string" },
        reason: { type: "string" }
      },
      ["x1", "y1", "x2", "y2", "layer", "reason"]
    ),
    functionTool(
      "cad_add_text",
      "Propose adding a CAD text note at a drawing coordinate.",
      {
        x: { type: "number" }, y: { type: "number" },
        text: { type: "string" },
        layer: { type: "string" },
        reason: { type: "string" }
      },
      ["x", "y", "text", "layer", "reason"]
    ),
    functionTool(
      "cad_replace_text",
      "Propose replacing the content of an existing text entity.",
      {
        sourceId: { type: "integer" },
        text: { type: "string" },
        reason: { type: "string" }
      },
      ["sourceId", "text", "reason"]
    ),
    functionTool(
      "cad_add_polyline",
      "Propose adding an open or closed polyline using exact drawing-unit coordinates. Use only coordinates supplied by the user or CAD-JSON vertices.",
      {
        points: {
          type: "array",
          items: { type: "array", items: { type: "number" }, minItems: 2, maxItems: 2 },
          minItems: 2,
          maxItems: 128
        },
        closed: { type: "boolean" },
        layer: { type: "string" },
        reason: { type: "string" }
      },
      ["points", "closed", "layer", "reason"]
    ),
    functionTool(
      "cad_offset_entity",
      "Propose a parallel/concentric offset of an existing LINE, CIRCLE or RECTANGLE. Distance is in drawing units and may be negative for the opposite side.",
      {
        sourceId: { type: "integer" },
        distance: { type: "number" },
        reason: { type: "string" }
      },
      ["sourceId", "distance", "reason"]
    ),
    functionTool(
      "cad_trim_line",
      "Propose trimming an existing LINE against another LINE. pickX/pickY is a drawing-unit point near the side of the target that should be removed. MusaCAD computes the exact intersection.",
      {
        targetSourceId: { type: "integer" },
        boundarySourceId: { type: "integer" },
        pickX: { type: "number" },
        pickY: { type: "number" },
        reason: { type: "string" }
      },
      ["targetSourceId", "boundarySourceId", "pickX", "pickY", "reason"]
    ),
    functionTool(
      "cad_extend_line",
      "Propose extending an existing LINE until it intersects another LINE segment. MusaCAD computes the exact intersection.",
      {
        targetSourceId: { type: "integer" },
        boundarySourceId: { type: "integer" },
        reason: { type: "string" }
      },
      ["targetSourceId", "boundarySourceId", "reason"]
    ),
    functionTool(
      "cad_continue_path",
      "Propose continuing an existing open LINE or POLYLINE from its start or end. points are ordered outward from the chosen existing endpoint and are in drawing units.",
      {
        sourceId: { type: "integer" },
        from: { type: "string", enum: ["start", "end"] },
        points: {
          type: "array",
          items: { type: "array", items: { type: "number" }, minItems: 2, maxItems: 2 },
          minItems: 1,
          maxItems: 128
        },
        reason: { type: "string" }
      },
      ["sourceId", "from", "points", "reason"]
    ),
    functionTool(
      "cad_add_pipe_note",
      "Propose adding a mechanical pipe annotation. diameter and slope may be empty individually, but at least one must contain a value such as DN100 or Eğim %2.",
      {
        x: { type: "number" },
        y: { type: "number" },
        diameter: { type: "string" },
        slope: { type: "string" },
        layer: { type: "string" },
        reason: { type: "string" }
      },
      ["x", "y", "diameter", "slope", "layer", "reason"]
    ),
    functionTool(
      "cad_insert_mechanical_block",
      "Propose inserting one approved built-in mechanical symbol. Coordinates are drawing units; scale is the MusaCAD block scale.",
      {
        blockId: {
          type: "string",
          enum: ["mec_pump","mec_valve","mec_fan","mec_radiator","mec_sprinkler","mec_diffuser","mec_grille","mec_fire_cabinet","mec_equipment_tag"]
        },
        x: { type: "number" },
        y: { type: "number" },
        scale: { type: "number" },
        rotation: { type: "number" },
        layer: { type: "string" },
        label: { type: "string" },
        reason: { type: "string" }
      },
      ["blockId", "x", "y", "scale", "rotation", "layer", "label", "reason"]
    )
  ];
}

function functionTool(name, description, properties, required) {
  return {
    type: "function",
    name,
    description,
    parameters: {
      type: "object",
      properties,
      required,
      additionalProperties: false
    },
    strict: true
  };
}

function parseOpenAiOutput(data) {
  const texts = [];
  const actions = [];
  const sourceMap = new Map();
  let webUsed = false;
  const output = data && Array.isArray(data.output) ? data.output : [];

  for (const item of output) {
    if (!item || typeof item !== "object") continue;
    if (item.type === "web_search_call") {
      webUsed = true;
      const sources = item.action && Array.isArray(item.action.sources) ? item.action.sources : [];
      for (const source of sources) {
        const url = String(source && source.url || "").trim();
        if (!/^https?:\/\//i.test(url)) continue;
        const title = String(source && source.title || "").trim();
        sourceMap.set(url, { title, url });
      }
    }

    if (item.type === "function_call") {
      const name = String(item.name || "").trim();
      if (!name) continue;
      let args = {};
      try { args = JSON.parse(String(item.arguments || "{}")); } catch (_) { args = {}; }
      actions.push({
        name,
        arguments: args,
        reason: typeof args.reason === "string" ? args.reason : ""
      });
      continue;
    }

    if (item.type === "message" && Array.isArray(item.content)) {
      for (const part of item.content) {
        if (part && part.type === "output_text" && typeof part.text === "string" && part.text.trim()) {
          texts.push(part.text.trim());
          const annotations = Array.isArray(part.annotations) ? part.annotations : [];
          for (const annotation of annotations) {
            if (!annotation || annotation.type !== "url_citation") continue;
            const citation = annotation.url_citation && typeof annotation.url_citation === "object"
              ? annotation.url_citation : annotation;
            const url = String(citation.url || "").trim();
            if (!/^https?:\/\//i.test(url)) continue;
            const title = String(citation.title || "").trim();
            sourceMap.set(url, { title, url });
          }
        }
      }
    }
  }

  const usage = data && data.usage && typeof data.usage === "object" ? data.usage : {};
  return {
    reply: texts.join("\n\n").trim(),
    actions,
    sources: Array.from(sourceMap.values()).slice(0, 20),
    webUsed,
    usage: {
      inputTokens: safeTokenCount(usage.input_tokens),
      outputTokens: safeTokenCount(usage.output_tokens),
      totalTokens: safeTokenCount(usage.total_tokens)
    }
  };
}

function parseGeminiChatOutput(data) {
  const actions = [];
  const texts = [];
  const choices = data && Array.isArray(data.choices) ? data.choices : [];
  const message = choices[0] && choices[0].message && typeof choices[0].message === "object" ? choices[0].message : {};
  const content = message.content;
  if (typeof content === "string" && content.trim()) texts.push(content.trim());
  else if (Array.isArray(content)) {
    for (const part of content) {
      if (part && typeof part.text === "string" && part.text.trim()) texts.push(part.text.trim());
    }
  }

  const calls = Array.isArray(message.tool_calls) ? message.tool_calls : [];
  for (const call of calls) {
    const fn = call && call.function && typeof call.function === "object" ? call.function : null;
    if (!fn) continue;
    const name = String(fn.name || "").trim();
    if (!name) continue;
    let args = {};
    try { args = JSON.parse(String(fn.arguments || "{}")); } catch (_) { args = {}; }
    actions.push({
      name,
      arguments: args,
      reason: typeof args.reason === "string" ? args.reason : ""
    });
  }

  const usage = data && data.usage && typeof data.usage === "object" ? data.usage : {};
  return {
    reply: texts.join("\n\n").trim(),
    actions,
    sources: [],
    webUsed: false,
    usage: {
      inputTokens: safeTokenCount(usage.prompt_tokens),
      outputTokens: safeTokenCount(usage.completion_tokens),
      totalTokens: safeTokenCount(usage.total_tokens)
    }
  };
}

function toChatCompletionsTool(tool) {
  return {
    type: "function",
    function: {
      name: tool.name,
      description: tool.description,
      parameters: tool.parameters
    }
  };
}

function selectAiProvider(env) {
  const requested = String(env.AI_PROVIDER || "").trim().toLowerCase();
  if (requested === "gemini" || (!requested && env.GEMINI_API_KEY)) {
    if (!env.GEMINI_API_KEY || !env.GEMINI_MODEL)
      return { ok: false, message: "Gemini AI worker is not fully configured" };
    return { ok: true, name: "gemini", model: String(env.GEMINI_MODEL) };
  }
  if (requested === "openai" || (!requested && env.OPENAI_API_KEY)) {
    if (!env.OPENAI_API_KEY || !env.OPENAI_MODEL)
      return { ok: false, message: "OpenAI AI worker is not fully configured" };
    return { ok: true, name: "openai", model: String(env.OPENAI_MODEL) };
  }
  return { ok: false, message: "No AI provider is configured" };
}

function safeTokenCount(value) {
  const n = Number(value);
  return Number.isFinite(n) && n >= 0 ? Math.floor(n) : 0;
}

async function verifySession(authHeader, publicKeyPem) {
  try {
    const match = /^Bearer\s+(.+)$/i.exec(String(authHeader || ""));
    if (!match) return { ok: false, message: "Missing AI session" };
    const token = match[1].trim();
    const parts = token.split(".");
    if (parts.length !== 3 || (parts[0] !== "MAI1" && parts[0] !== "MAI2"))
      return { ok: false, message: "Invalid AI session" };

    const payloadBytes = decodeBase64url(parts[1]);
    const signatureBytes = decodeBase64url(parts[2]);
    const payload = new TextDecoder().decode(payloadBytes);
    const fields = payload.split("|");

    let mode = "licensed";
    if (parts[0] === "MAI1") {
      if (fields.length !== 3 || fields[0] !== "MAI1")
        return { ok: false, message: "Invalid AI session payload" };
    } else {
      if (fields.length !== 4 || fields[0] !== "MAI2")
        return { ok: false, message: "Invalid AI session payload" };
      mode = fields[3] === "developer" ? "developer" : "licensed";
    }

    const deviceId = fields[1];
    const expiresAtMs = Number(fields[2]);
    if (!validDeviceId(deviceId) || !Number.isSafeInteger(expiresAtMs) || expiresAtMs <= Date.now())
      return { ok: false, message: "AI session expired" };

    const key = await crypto.subtle.importKey(
      "spki",
      pemBytes(publicKeyPem, "PUBLIC KEY"),
      { name: "RSASSA-PKCS1-v1_5", hash: "SHA-256" },
      false,
      ["verify"]
    );
    const valid = await crypto.subtle.verify("RSASSA-PKCS1-v1_5", key, signatureBytes, payloadBytes);
    return valid ? { ok: true, deviceId, expiresAtMs, mode } : { ok: false, message: "Invalid AI session signature" };
  } catch (_) {
    return { ok: false, message: "Invalid AI session" };
  }
}

async function readJson(request) {
  try {
    const text = await request.text();
    if (new TextEncoder().encode(text).length > MAX_BODY_BYTES)
      return { ok: false, response: json({ status: "denied", message: "request too large" }, 413) };
    return { ok: true, body: JSON.parse(text) };
  } catch (_) {
    return { ok: false, response: json({ status: "denied", message: "invalid json" }, 400) };
  }
}

function validDeviceId(value) {
  return /^MC-[0-9A-F]{8}-[0-9A-F]{8}-[0-9A-F]{8}$/.test(String(value || ""))
    || /^MC-FALLBACK-[0-9A-F-]{36}$/.test(String(value || ""));
}

function positiveInt(value, fallback, min, max) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) return fallback;
  return Math.max(min, Math.min(max, Math.floor(parsed)));
}

function pemBytes(pem, label) {
  const normalized = String(pem || "").replace(/\\n/g, "\n");
  const clean = normalized
    .replace(`-----BEGIN ${label}-----`, "")
    .replace(`-----END ${label}-----`, "")
    .replace(/\s/g, "");
  if (!clean) throw new Error("missing key");
  const binary = atob(clean);
  const out = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) out[i] = binary.charCodeAt(i);
  return out;
}

function decodeBase64url(value) {
  const normalized = String(value || "").replace(/-/g, "+").replace(/_/g, "/");
  const padded = normalized + "=".repeat((4 - normalized.length % 4) % 4);
  const binary = atob(padded);
  const out = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) out[i] = binary.charCodeAt(i);
  return out;
}

function json(body, status = 200, extraHeaders = {}) {
  return new Response(JSON.stringify(body), {
    status,
    headers: {
      "content-type": "application/json; charset=utf-8",
      "cache-control": "no-store",
      "x-content-type-options": "nosniff",
      ...extraHeaders
    }
  });
}

export { parseOpenAiOutput, parseGeminiChatOutput, cadProposalTools, verifySession };
