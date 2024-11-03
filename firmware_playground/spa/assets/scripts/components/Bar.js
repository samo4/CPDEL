import { nextTick, ref, toRef, toRefs, computed, defineComponent, watch, reactive, onMounted, inject, onBeforeUnmount } from 'vue'

export default {
  template: `
    <canvas ref="canvasEl"></canvas>
  `,
  props: {
    x: {
      type: Array, // of strings
      required: true
    },
    y: {
      type: Array,
      required: true
    }
  },
  setup(props) {
    const canvasEl = ref(null)

    const drawGraph = () => {
      const canvas = canvasEl.value
      if (!canvas) return

      const ctx = canvas.getContext('2d')
      ctx.clearRect(0, 0, canvas.width, canvas.height)

      const barWidth = canvas.width / props.x.length
      const maxY = Math.max(...props.y)


      props.y.forEach((value, index) => {
        const barHeight = (value / maxY) * canvas.height
        const threshold = canvas.height - barHeight - 100
        ctx.fillStyle = 'blue'
        ctx.fillRect(index * barWidth, canvas.height - barHeight, barWidth - 1, barHeight)

        const textColor = barHeight > threshold ? 'white' : 'black'
        ctx.fillStyle = textColor
        ctx.textAlign = 'center'
        ctx.font = '12px Arial'
        ctx.fillText(props.x[index], index * barWidth + barWidth / 2, canvas.height - 5)

        ctx.fillStyle = textColor
        ctx.font = '14px Arial'
        // ctx.fillText(value, index * barWidth + barWidth / 2, canvas.height - barHeight - 10)
		    ctx.fillText(value, index * barWidth + barWidth / 2, canvas.height - 20)
      })
    }

    watch(() => [props.x, props.y], drawGraph, { deep: true })

    onMounted(() => {
      drawGraph()
    })

    return {
      canvasEl
    }
  }
}
