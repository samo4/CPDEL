
const elError = document.getElementById("error")

const ctx1 = document.getElementById("chart-1")
const ctx2 = document.getElementById("chart-2")

const chartOptions1 = {
  type: "scatter",
  data: {
    labels: ["I"],
    datasets: [
      {
        label: "U",
        yAxisID: 'A',
        borderColor: "rgba(255, 99, 132, 0.2)",
        showLine: true,
        data: [],
      },
      {
        label: "I",
        yAxisID: 'B',
        borderColor: "rgba(99, 255, 132, 0.2)",
        showLine: true,
        data: [],
      }
    ],
  },
  options: {
    /*parsing: {
        xAxisKey: 'ds',
        yAxisKey: 'voltage'
    },*/
    scales: {
      x: {
        type: 'linear',
        position: 'bottom'
      },
      A: {
        type: 'linear',
        position: 'left',
        beginAtZero: true
      },
      B: {
        type: 'linear',
        position: 'right',
        beginAtZero: true
      }
    },
  },
}

const chartOptions2 = {
  type: "scatter",
  data: {
    labels: ["I"],
    datasets: [
      {
        label: "U",
        yAxisID: 'A',
        borderColor: "rgba(255, 99, 132, 0.2)",
        showLine: true,
        data: [],
      },
      {
        label: "I",
        yAxisID: 'B',
        borderColor: "rgba(99, 255, 132, 0.2)",
        showLine: true,
        data: [],
      }
    ],
  },
  options: {
    /*parsing: {
        xAxisKey: 'ds',
        yAxisKey: 'voltage'
    },*/
    scales: {
      x: {
        type: 'linear',
        position: 'bottom'
      },
      A: {
        type: 'linear',
        position: 'left',
        beginAtZero: true
      },
      B: {
        type: 'linear',
        position: 'right',
        beginAtZero: true
      }
    },
  },
}

const chart1 = new Chart(ctx1, chartOptions1)
const chart2 = new Chart(ctx2, chartOptions2)

const fetchStatus = async (id) => {
  try {
    const response = await fetch( `/status?a=${id}`)
    const a = await response.json()
    document.querySelector(`#current-${id}`).innerHTML = a.current
    document.querySelector(`#voltage-${id}`).innerHTML = a.voltage
  } catch (e) {
    elError.innerHTML += e + "\n"
    console.error(e)
  }
}

const currentCommandAsync = async (id) => {
  try {
    const value = document.getElementById(`data-command_current-${id}`).value
    const response = await fetch( `/status?a=${id}&v=${value/1000}&p=command_current`, { method: "PATCH" })
    if (!response.ok) throw await response.text()
    const a = await response.text()
    console.log(a)
  } catch (e) {
    elError.innerHTML += e + "\n"
    console.error(e)
  }
}

const toggleEnableAsync = async (id) => {
  try {
    const response = await fetch( `/status?a=${id}&v=toggle&p=enable`, { method: "PATCH" })
    const a = await response.text()
    console.log(a)
  } catch (e) {
    elError.innerHTML += e + "\n"
    console.error(e)
  }
}

window.onload = async e => {
  console.log('loaded')
  // setInterval(() => { fetchStatus(1); fetchStatus(2) }, 15000)
  document.getElementById("toggle-1").addEventListener('click', e => toggleEnableAsync(1))
  document.getElementById("toggle-2").addEventListener('click', e => toggleEnableAsync(2))
  document.getElementById("command_current-1").addEventListener('click', e => currentCommandAsync(1))
  document.getElementById("command_current-2").addEventListener('click', e => currentCommandAsync(2))
}


if (!!window.EventSource) {
  const source = new EventSource('/events')

  source.addEventListener('open', e => {
    console.log("Events Connected")
  }, false)

  source.addEventListener('error', e => {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected")
    }
  }, false)

  source.addEventListener('message', e => {
    console.log("message", e.data)
  }, false)

  source.addEventListener('new-data', e => {
    // console.log("new-data", e.data)
    const obj = JSON.parse(e.data)

    if (obj.address == 1) {
      chart1.data.datasets[0].data.push({ x: (obj.ds) / 10, y: obj.voltage })
      chart1.data.datasets[1].data.push({ x: (obj.ds) / 10, y: obj.current })
      chart1.update()
    } else if (obj.address == 2){
      chart2.data.datasets[0].data.push({ x: (obj.ds) / 10, y: obj.voltage })
      chart2.data.datasets[1].data.push({ x: (obj.ds) / 10, y: obj.current })
      chart2.update()
    }

    document.querySelector(`#current-${obj.address}`).innerHTML = obj.current
    document.querySelector(`#voltage-${obj.address}`).innerHTML = obj.voltage

  }, false);
}
