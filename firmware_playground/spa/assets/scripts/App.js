
import { ref, toRef, toRefs, computed, defineComponent, watch, reactive, onMounted, inject, onBeforeUnmount } from 'vue'
import wsService from './websocketService'

export default App = {
  template: `
  <aside>
    <dl v-for="s in stats">
      <dt>{{ s.name }}</dt>
      <dd>{{ s.value }}</dd>
    </dl>
  </aside>
  <main>
    <div class="cards">
      <Card v-for="card in [...charts, ...cards]" :card="card" />
    </div>
  </main>

  `,
  setup() {

    const cards = ref([])
    const charts = ref([])
    const stats = ref([])
    let pingInterval = null

    onMounted(() => {
      wsService.connect()
      wsService.onMessage(m => {
        if (m.detail.command === "update:layout:begin") {
        } else if (m.detail.command === "update:layout:next") {
          if (m.detail.cards) {
            cards.value = m.detail.cards.map(card => {
              let t = card.t
              if (card.n.startsWith('Log')) {
                t = 'appendable'
              }
              return {
              id: card.id,
              name: card.n,
              type: t,
              symbol: card.s,
              value: t == 'appendable' ? '' : card.v,
              ...(card.min !== undefined && { min: card.min }),
              ...(card.max !== undefined && { max: card.max }),
              ...(card.step !== undefined && { step: card.step })
            }})
          } else if (m.detail.stats) {
            stats.value = m.detail.stats.map(card => { return {
              id: card.i,
              name: card.k,
              value: card.v
            }})
          } else if (m.detail.charts) {
            charts.value = m.detail.charts.map(chart => {
              let type = chart.t
              if (chart.n.startsWith('Line')) {
                type = 'line'
              }
              return {
                id: chart.id,
                name: chart.n,
                type,
                x: chart.x,
                y: chart.y
              }
            })
          }
        } else if (m.detail.command === "update:components") {
          if (m.detail.cards) {
            m.detail.cards.forEach(card => {
              const cardToUpdate = cards.value.find(c => c.id === card.id)
              if (!cardToUpdate) {
                return
              }
              if (cardToUpdate.type === 'appendable') {
                cardToUpdate.value += card.v
              } else {
                cardToUpdate.value = card.v
              }
            })
          }
          if (m.detail.charts) {
            m.detail.charts.forEach(chart => {
              const chartToUpdate = charts.value.find(c => c.id === chart.id)
              if (!chartToUpdate) {
                return
              }
              if (chart.x) {
                chartToUpdate.x = chart.x
              }
              if (chart.y) {
                chartToUpdate.y = chart.y
              }
            })
          }
        } else if (m.detail.command === "pong") {
          console.log('Pong')
        }
        else {
          console.log('Unknown command', m.detail.command)
        }
      })

      wsService.onConnected(() => {
        wsService.sendMessage({ "command": "get:layout" })
        // pingInterval = setInterval(() => {
        //   wsService.sendMessage({ "command": "ping" })
        // }, 2000)
      })
    })

    onBeforeUnmount(() => {
      wsService.close()
      if (pingInterval) {
        clearInterval(pingInterval)
      }
    })

    return {
      stats,
      cards,
      charts
    }
  }
}
