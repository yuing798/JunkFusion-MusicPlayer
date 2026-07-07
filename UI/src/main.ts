import { createApp } from 'vue'
import App from './App.vue'
import { createPinia } from 'pinia'

import './assets/css/theme.css'
import './bridge/initBridge.ts'//注册juce监听器

const pinia = createPinia()

createApp(App).use(pinia).mount('#app')
