import { nextTick, ref, toRef, toRefs, computed, defineComponent, watch, reactive, onMounted, inject, onBeforeUnmount } from 'vue'

import Line from './Line'
import Bar from './Bar'
import wsService from '../websocketService'

export default Card = {
  components: { Bar, Line },
  template: `
  <div class="card" v-if="card.type=='generic'" ref="cardEl">
    <i>📋</i>
    <h5>{{ card.name }}</h5>
    <p>{{ card.value }} <small>{{ card.symbol }}</small></p>
  </div>
  <div class="card" v-else-if="card.type=='temperature'">
    <i>🌡️</i>
    <h5>{{ card.name }}</h5>
    <p>{{ card.value }} <small>{{ card.symbol }}</small></p>
  </div>
  <div class="card" v-else-if="card.type=='humidity'">
    <i>💧</i>
    <h5>{{ card.name }}</h5>
    <p>{{ card.value }} <small>{{ card.symbol }}</small></p>
  </div>
  <div class="card" v-else-if="card.type=='status'">
    <i v-if="card.symbol==='i'">💤</i>
    <i v-else-if="card.symbol==='w'">🟡</i>
    <i v-else-if="card.symbol==='s'">✅</i>
    <i v-else-if="card.symbol==='d'">⚠️</i>
    <i v-else>❓</i>
    <h5>{{ card.name }}</h5>
    <p>{{ card.value }}</p>
  </div>
  <div class="card" v-else-if="card.type=='progress'">
    <i>📊</i>
    <h5>{{ card.name }}</h5>
    <p>
      <progress :value="card.value" max="100"></progress>
      <small>{{ card.value }}%</small>
    </p>
  </div>
  <div class="card" v-else-if="card.type=='slider'">
    <i>🎚️</i>
    <h5>{{ card.name }}</h5>
    <p>
      <input type="range" v-model="card.value" :min="card.min" :max="card.max" @change="changeSlider" />
    </p>
  </div>
  <div class="card" v-else-if="card.type=='button'">
    <i>🔘</i>
    <p><button @click="clickButton">{{ card.name }}</button></p>
  </div>
  <div class="card appendable" v-else-if="card.type=='appendable'" ref="cardEl">
    <h5>{{ card.name }}</h5>
    <textarea ref="textareaEl" disabled rows=10>{{ card.value }}</textarea>
  </div>
  <div class="card chart" v-else-if="card.type=='bar'" ref="cardEl">
    <h5>{{ card.name }}</h5>
    <Bar :x="card.x" :y="card.y" />
  </div>
  <div class="card chart" v-else-if="card.type=='line'" ref="cardEl">
    <h5>{{ card.name }}</h5>
    <Line :x="card.x" :y="card.y" />
  </div>
  <div class="card" v-else>
    <i>❓</i>
    <h5>Unknown card</h5>
    <p>{{ card }}</p>
  </div>
  `,
  props: {
    card: { type: Object, required: true }
  },
  setup(props) {
    const textareaEl = ref(null)

    const clickButton = () => {
      wsService.sendMessage({ command: 'button:clicked', id: props.card.id, value: props.card.value ? 0 : 1 })
    }

    const changeSlider = () => {
      wsService.sendMessage({ command: 'slider:changed', id: props.card.id, value: props.card.value })
    }

    watch(() => props.card.value, (newValue) => {
      if (textareaEl.value) {
        textareaEl.value.scrollTop = textareaEl.value.scrollHeight
      }
    })

    return {
      textareaEl,
      changeSlider,
      clickButton
    }
  }
}
