import { nextTick, ref, toRef, toRefs, computed, defineComponent, watch, reactive, onMounted, inject, onBeforeUnmount } from 'vue'

export default {
  template: `
    <canvas ref="canvasEl"></canvas>
  `,
  props: {
    x: {
      type: Array,
      required: true
    },
    y: {
      type: Array,
      required: true
    }
  },
  setup(props) {
    const canvasEl = ref(null)

    const padding = 20

    const drawGraph = () => {
      const canvas = canvasEl.value
      if (!canvas) return

      const ctx = canvas.getContext('2d')
      ctx.clearRect(0, 0, canvas.width, canvas.height)
      ctx.strokeStyle = '#000'
      ctx.lineWidth = 2
      ctx.beginPath()

      const maxX = Math.max(...props.x)
      const maxY = Math.max(...props.y)
      const minY = Math.min(...props.y)

      const points = props.x.map((x, index) => [
        padding + (x / maxX) * (canvas.width - 2 * padding),
        canvas.height - padding - (props.y[index] / maxY) * (canvas.height - 2 * padding)
      ])

      points.forEach((point, index) => {
        const [x, y] = point
        if (index === 0) {
          ctx.moveTo(x, y)
        } else {
          ctx.lineTo(x, y)
        }
      })
      ctx.stroke()

      ctx.fillStyle = 'black'
      ctx.textAlign = 'center'
      ctx.font = '12px Arial'
      const xLabels = [props.x[0], props.x[Math.floor(props.x.length / 2)], props.x[props.x.length - 1]]
      const xPositions = [0, Math.floor(props.x.length / 2), props.x.length - 1]
      xPositions.forEach((pos, index) => {
        const x = padding + (pos / (props.x.length - 1)) * (canvas.width - 2 * padding)
        ctx.fillText(xLabels[index], x, canvas.height - 5)
      })

      const yLabels = [minY, maxY]
      const yPositions = [canvas.height - padding, padding]
      yLabels.forEach((label, index) => {
        ctx.fillText(label, padding - 10, yPositions[index])
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
