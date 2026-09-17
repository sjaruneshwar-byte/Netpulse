let currentAgents = [];

// --------------------------------------------------
// Fetch JSON from API
// --------------------------------------------------

async function fetchJSON(url) {
  const response = await fetch(url);

  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`);
  }

  return response.json();
}

// --------------------------------------------------
// Format numbers
// --------------------------------------------------

function formatNumber(value, decimals = 2) {
  const number = Number(value);

  if (!Number.isFinite(number)) {
    return "-";
  }

  return number.toFixed(decimals);
}

// --------------------------------------------------
// Escape HTML
// --------------------------------------------------

function escapeHTML(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

// --------------------------------------------------
// Format Unix timestamp
// --------------------------------------------------

function formatTimestamp(timestamp) {
  const number = Number(timestamp);

  if (!Number.isFinite(number)) {
    return "-";
  }

  const date = new Date(number * 1000);

  if (Number.isNaN(date.getTime())) {
    return "-";
  }

  return date.toLocaleString();
}

// --------------------------------------------------
// API status
// --------------------------------------------------

async function updateAPIStatus() {
  const element = document.getElementById("apiStatus");

  try {
    await fetchJSON("/api/health");

    element.textContent = "● API ONLINE";

    element.classList.remove("offline");

    element.classList.add("online");
  } catch (error) {
    element.textContent = "● API OFFLINE";

    element.classList.remove("online");

    element.classList.add("offline");
  }
}

// --------------------------------------------------
// Summary
// --------------------------------------------------

async function updateSummary() {
  const data = await fetchJSON("/api/summary");

  document.getElementById("totalAgents").textContent = data.total_agents ?? 0;

  document.getElementById("healthyAgents").textContent =
    data.healthy_agents ?? 0;

  document.getElementById("suspectedAgents").textContent =
    data.suspected_agents ?? 0;

  document.getElementById("offlineAgents").textContent =
    data.offline_agents ?? 0;

  document.getElementById("faultCount").textContent = data.fault_count ?? 0;

  document.getElementById("telemetryCount").textContent =
    data.telemetry_count ?? 0;
}

// --------------------------------------------------
// Agents
// --------------------------------------------------

async function updateAgents() {
  const agents = await fetchJSON("/api/agents");

  currentAgents = Array.isArray(agents) ? agents : [];

  const container = document.getElementById("agentCards");

  if (currentAgents.length === 0) {
    container.innerHTML = `
            <div class="empty-state">
                No agents registered.
            </div>
            `;

    updateAgentSelector([]);

    return;
  }

  container.innerHTML = currentAgents.map(createAgentCard).join("");

  updateAgentSelector(currentAgents);
}

// --------------------------------------------------
// Create agent card
// --------------------------------------------------

function createAgentCard(agent) {
  const status = String(agent.status || "OFFLINE").toUpperCase();

  const statusClass = status.toLowerCase();

  const statusIcon =
    status === "HEALTHY" ? "●" : status === "SUSPECTED" ? "▲" : "■";

  return `
        <article class="agent-card">

            <div class="agent-header">

                <div>

                    <div class="agent-name">
                        ${escapeHTML(agent.agent_id)}
                    </div>

                    <div class="agent-host">
                        ${escapeHTML(agent.hostname || "-")}
                    </div>

                </div>


                <div
                    class="status ${statusClass}"
                >
                    ${statusIcon}
                    ${escapeHTML(status)}
                </div>

            </div>


            <div class="agent-details">

                <div class="metric">

                    <span class="metric-label">
                        CPU
                    </span>

                    <span class="metric-value">
                        ${formatNumber(agent.cpu)}%
                    </span>

                </div>


                <div class="metric">

                    <span class="metric-label">
                        Memory
                    </span>

                    <span class="metric-value">
                        ${formatNumber(agent.memory)}%
                    </span>

                </div>


                <div class="metric">

                    <span class="metric-label">
                        Interface
                    </span>

                    <span class="metric-value">
                        ${escapeHTML(agent.interface_name || "-")}
                    </span>

                </div>


                <div class="metric">

                    <span class="metric-label">
                        IP
                    </span>

                    <span class="metric-value">
                        ${escapeHTML(agent.client_ip || "-")}
                    </span>

                </div>

            </div>


            <div class="agent-footer">

                <span>
                    Last seen:
                </span>

                <span>
                    ${formatTimestamp(agent.last_seen)}
                </span>

            </div>

        </article>
    `;
}

// --------------------------------------------------
// Agent selector
// --------------------------------------------------

function updateAgentSelector(agents) {
  const selector = document.getElementById("agentSelector");

  const previousValue = selector.value;

  selector.innerHTML = `
        <option value="">
            Select an agent
        </option>
        `;

  agents.forEach((agent) => {
    const option = document.createElement("option");

    option.value = agent.agent_id;

    option.textContent = agent.agent_id;

    selector.appendChild(option);
  });

  if (
    previousValue &&
    agents.some((agent) => agent.agent_id === previousValue)
  ) {
    selector.value = previousValue;
  }
}

// --------------------------------------------------
// Topology
// --------------------------------------------------

async function updateTopology() {
  const data = await fetchJSON("/api/topology");

  const container = document.getElementById("topologyContainer");

  const agents = Array.isArray(data.agents) ? data.agents : [];

  if (agents.length === 0) {
    container.innerHTML = `
            <div class="empty-state">
                No agents registered.
            </div>
            `;

    return;
  }

  const width = 1000;

  const columns = Math.min(4, Math.max(1, agents.length));

  const rows = Math.ceil(agents.length / columns);

  const rowHeight = 130;

  const height = Math.max(430, 220 + rows * rowHeight);

  const serverX = width / 2;

  const serverY = 70;

  let svg = `
        <svg
            viewBox="0 0 ${width} ${height}"
            class="topology-svg"
            xmlns="http://www.w3.org/2000/svg"
        >

            <defs>

                <marker
                    id="topologyArrow"
                    markerWidth="10"
                    markerHeight="10"
                    refX="8"
                    refY="3"
                    orient="auto"
                >

                    <path
                        d="M0,0 L0,6 L9,3 z"
                        fill="#9ca3af"
                    />

                </marker>

            </defs>


            <rect
                x="${serverX - 120}"
                y="${serverY - 32}"
                width="240"
                height="64"
                rx="12"
                class="topology-server"
            />


            <text
                x="${serverX}"
                y="${serverY + 6}"
                class="topology-server-text"
                text-anchor="middle"
            >
                NETPULSE SERVER
            </text>
    `;

  agents.forEach((agent, index) => {
    const column = index % columns;

    const row = Math.floor(index / columns);

    const x =
      columns === 1 ? serverX : 130 + column * ((width - 260) / (columns - 1));

    const y = 220 + row * rowHeight;

    const status = String(agent.status || "OFFLINE").toUpperCase();

    const statusClass = status.toLowerCase();

    const symbol =
      status === "HEALTHY" ? "●" : status === "SUSPECTED" ? "▲" : "■";

    svg += `

                <line
                    x1="${serverX}"
                    y1="${serverY + 32}"
                    x2="${x}"
                    y2="${y - 40}"
                    class="topology-link"
                    marker-end="url(#topologyArrow)"
                />


                <rect
                    x="${x - 100}"
                    y="${y - 40}"
                    width="200"
                    height="88"
                    rx="12"
                    class="topology-node ${statusClass}"
                />


                <text
                    x="${x}"
                    y="${y - 12}"
                    class="topology-agent-name"
                    text-anchor="middle"
                >
                    ${escapeHTML(agent.agent_id)}
                </text>


                <text
                    x="${x}"
                    y="${y + 10}"
                    class="topology-agent-status ${statusClass}"
                    text-anchor="middle"
                >
                    ${symbol}
                    ${escapeHTML(status)}
                </text>


                <text
                    x="${x}"
                    y="${y + 32}"
                    class="topology-agent-ip"
                    text-anchor="middle"
                >
                    ${escapeHTML(agent.client_ip || "-")}
                </text>

            `;
  });

  svg += `
        </svg>
    `;

  container.innerHTML = svg;
}

// --------------------------------------------------
// Historical telemetry
// --------------------------------------------------

async function updateCharts() {
  const selector = document.getElementById("agentSelector");

  const agentId = selector.value;

  if (!agentId) {
    clearChart("systemChart", "Select an agent");

    clearChart("networkChart", "Select an agent");

    return;
  }

  const data = await fetchJSON(
    `/api/telemetry?agent_id=${encodeURIComponent(agentId)}&limit=50`,
  );

  const orderedData = [...data].reverse();

  drawSystemChart(orderedData);

  drawNetworkChart(orderedData);
}

// --------------------------------------------------
// System chart
// --------------------------------------------------

function drawSystemChart(data) {
  const canvas = document.getElementById("systemChart");

  const context = canvas.getContext("2d");

  prepareCanvas(canvas, context);

  if (data.length === 0) {
    drawEmptyMessage(canvas, context, "No telemetry data available");

    return;
  }

  const cpuValues = data.map((row) => Number(row.cpu) || 0);

  const memoryValues = data.map((row) => Number(row.memory) || 0);

  const maxValue = 100;

  drawMultiLineChart(
    canvas,
    context,
    [
      {
        values: cpuValues,
        label: "CPU",
      },
      {
        values: memoryValues,
        label: "Memory",
      },
    ],
    maxValue,
  );
}

// --------------------------------------------------
// Network chart
// --------------------------------------------------

function drawNetworkChart(data) {
  const canvas = document.getElementById("networkChart");

  const context = canvas.getContext("2d");

  prepareCanvas(canvas, context);

  if (data.length === 0) {
    drawEmptyMessage(canvas, context, "No telemetry data available");

    return;
  }

  const rxValues = data.map((row) => Number(row.rx_bps) || 0);

  const txValues = data.map((row) => Number(row.tx_bps) || 0);

  const maxValue = Math.max(...rxValues, ...txValues, 1);

  drawMultiLineChart(
    canvas,
    context,
    [
      {
        values: rxValues,
        label: "RX",
      },
      {
        values: txValues,
        label: "TX",
      },
    ],
    maxValue,
  );
}

// --------------------------------------------------
// Draw multiple series
// --------------------------------------------------

function drawMultiLineChart(canvas, context, series, maxValue) {
  const width = canvas.width;

  const height = canvas.height;

  const paddingLeft = 60;

  const paddingRight = 25;

  const paddingTop = 35;

  const paddingBottom = 45;

  const chartWidth = width - paddingLeft - paddingRight;

  const chartHeight = height - paddingTop - paddingBottom;

  // ----------------------------------------------
  // Grid
  // ----------------------------------------------

  context.strokeStyle = "#e5e7eb";

  context.lineWidth = 1;

  context.fillStyle = "#6b7280";

  context.font = "11px Arial";

  for (let i = 0; i <= 5; i++) {
    const y = paddingTop + chartHeight - (chartHeight * i) / 5;

    context.beginPath();

    context.moveTo(paddingLeft, y);

    context.lineTo(width - paddingRight, y);

    context.stroke();

    const value = (maxValue * i) / 5;

    context.fillText(formatCompactNumber(value), 8, y + 4);
  }

  // ----------------------------------------------
  // Series
  // ----------------------------------------------

  const linePatterns = [
    {
      dash: [],
      marker: "circle",
    },
    {
      dash: [8, 5],
      marker: "square",
    },
  ];

  series.forEach((item, seriesIndex) => {
    const values = item.values;

    const pattern = linePatterns[seriesIndex % linePatterns.length];

    context.beginPath();

    context.strokeStyle = "#2563eb";

    context.lineWidth = seriesIndex === 0 ? 2.5 : 2;

    context.setLineDash(pattern.dash);

    values.forEach((value, index) => {
      let x;

      if (values.length <= 1) {
        x = paddingLeft + chartWidth / 2;
      } else {
        x = paddingLeft + (chartWidth * index) / (values.length - 1);
      }

      const boundedValue = Math.max(0, Math.min(value, maxValue));

      const y =
        paddingTop + chartHeight - (boundedValue / maxValue) * chartHeight;

      if (index === 0) {
        context.moveTo(x, y);
      } else {
        context.lineTo(x, y);
      }
    });

    context.stroke();

    context.setLineDash([]);
  });

  // ----------------------------------------------
  // Legend
  // ----------------------------------------------

  const legendY = height - 18;

  series.forEach((item, index) => {
    const x = paddingLeft + index * 100;

    context.fillStyle = "#2563eb";

    context.fillRect(x, legendY - 8, 18, 3);

    context.fillStyle = "#374151";

    context.font = "12px Arial";

    context.fillText(item.label, x + 25, legendY);
  });

  // ----------------------------------------------
  // Y-axis maximum
  // ----------------------------------------------

  context.fillStyle = "#6b7280";

  context.font = "11px Arial";

  context.fillText(formatCompactNumber(maxValue), 8, paddingTop);
}

// --------------------------------------------------
// Prepare canvas
// --------------------------------------------------

function prepareCanvas(canvas, context) {
  context.clearRect(0, 0, canvas.width, canvas.height);

  context.fillStyle = "#ffffff";

  context.fillRect(0, 0, canvas.width, canvas.height);
}

// --------------------------------------------------
// Empty chart
// --------------------------------------------------

function clearChart(id, message) {
  const canvas = document.getElementById(id);

  const context = canvas.getContext("2d");

  prepareCanvas(canvas, context);

  drawEmptyMessage(canvas, context, message);
}

function drawEmptyMessage(canvas, context, message) {
  context.fillStyle = "#6b7280";

  context.font = "16px Arial";

  context.textAlign = "center";

  context.fillText(message, canvas.width / 2, canvas.height / 2);

  context.textAlign = "left";
}

// --------------------------------------------------
// Compact number formatting
// --------------------------------------------------

function formatCompactNumber(value) {
  const number = Number(value);

  if (!Number.isFinite(number)) {
    return "-";
  }

  if (Math.abs(number) >= 1024 * 1024) {
    return (number / (1024 * 1024)).toFixed(1) + "M";
  }

  if (Math.abs(number) >= 1024) {
    return (number / 1024).toFixed(1) + "K";
  }

  return number.toFixed(0);
}

// --------------------------------------------------
// Recent telemetry
// --------------------------------------------------

async function updateTelemetry() {
  const data = await fetchJSON("/api/telemetry?limit=10");

  const table = document.getElementById("telemetryTable");

  if (!Array.isArray(data) || data.length === 0) {
    table.innerHTML = `
            <tr>

                <td
                    colspan="10"
                    class="empty-state"
                >
                    No telemetry records found.
                </td>

            </tr>
            `;

    return;
  }

  table.innerHTML = data
    .map(
      (row) =>
        `
                <tr>

                    <td>
                        ${escapeHTML(row.agent_id)}
                    </td>


                    <td>
                        ${escapeHTML(row.interface_name || "-")}
                    </td>


                    <td>
                        ${formatNumber(row.cpu)}%
                    </td>


                    <td>
                        ${formatNumber(row.memory)}%
                    </td>


                    <td>
                        ${formatNumber(row.rx_bps)}
                    </td>


                    <td>
                        ${formatNumber(row.tx_bps)}
                    </td>


                    <td>
                        ${row.rx_errors ?? 0}
                    </td>


                    <td>
                        ${row.tx_errors ?? 0}
                    </td>


                    <td>
                        ${row.rx_drops ?? 0}
                    </td>


                    <td>
                        ${row.tx_drops ?? 0}
                    </td>

                </tr>
                `,
    )
    .join("");
}

// --------------------------------------------------
// Recent faults
// --------------------------------------------------

async function updateFaults() {
  const data = await fetchJSON("/api/faults?limit=10");

  const table = document.getElementById("faultTable");

  if (!Array.isArray(data) || data.length === 0) {
    table.innerHTML = `
            <tr>

                <td
                    colspan="6"
                    class="empty-state"
                >
                    No fault events recorded.
                </td>

            </tr>
            `;

    return;
  }

  table.innerHTML = data
    .map(
      (row) =>
        `
                <tr>

                    <td>
                        ${formatTimestamp(row.timestamp)}
                    </td>


                    <td>
                        ${escapeHTML(row.agent_id)}
                    </td>


                    <td>
                        ${escapeHTML(row.interface_name || "-")}
                    </td>


                    <td>
                        ${escapeHTML(row.fault_type)}
                    </td>


                    <td>

                        <span
                            class="severity-badge severity-${escapeHTML(
                              row.severity,
                            )}"
                        >
                            ${escapeHTML(row.severity)}
                        </span>

                    </td>


                    <td>
                        ${escapeHTML(row.description)}
                    </td>

                </tr>
                `,
    )
    .join("");
}

// --------------------------------------------------
// Agent selector event
// --------------------------------------------------

document
  .getElementById("agentSelector")
  .addEventListener("change", updateCharts);

// --------------------------------------------------
// Refresh dashboard
// --------------------------------------------------

async function refreshDashboard() {
  try {
    await updateAPIStatus();

    await updateSummary();

    await updateAgents();

    await updateTopology();

    await updateTelemetry();

    await updateFaults();

    await updateCharts();

    document.getElementById("lastUpdated").textContent =
      "Updated " + new Date().toLocaleTimeString();
  } catch (error) {
    console.error("Dashboard update failed:", error);
  }
}

// --------------------------------------------------
// Initial load
// --------------------------------------------------

refreshDashboard();

// --------------------------------------------------
// Automatic refresh
// --------------------------------------------------

setInterval(refreshDashboard, 3000);
