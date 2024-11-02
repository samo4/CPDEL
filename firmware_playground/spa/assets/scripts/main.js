import { createApp } from 'vue'
import App from './App.js'
import Card from './components/Card.js'

const app = createApp(App)

app.component('Card', Card)
app.mount('#app')
